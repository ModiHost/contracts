#include <modihost.hpp>

namespace eosio {


   void token::create( const name&   issuer,
                     const asset&  maximum_supply )
   {
      require_auth( get_self() );

      auto sym = maximum_supply.symbol;
      check( sym.is_valid(), "invalid symbol name" );
      check( maximum_supply.is_valid(), "invalid supply");
      check( maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable( get_self(), sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      check( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( get_self(), [&]( auto& s ) {
         s.supply.symbol = maximum_supply.symbol;
         s.max_supply    = maximum_supply;
         s.issuer        = issuer;
      });
   }

   void token::issue( const name& to, const asset& quantity, const string& memo )
   {
      auto sym = quantity.symbol;
      check( sym.is_valid(), "invalid symbol name" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( get_self(), sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      check( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;
      check( to == st.issuer, "tokens can only be issued to issuer account" );

      require_auth( st.issuer );
      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.amount > 0, "must issue positive quantity" );

      check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify( st, same_payer, [&]( auto& s ) {
         s.supply += quantity;
      });

      add_balance( st.issuer, quantity, st.issuer );
   }

   void token::retire( const asset& quantity, const string& memo )
   {
      auto sym = quantity.symbol;
      check( sym.is_valid(), "invalid symbol name" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( get_self(), sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      check( existing != statstable.end(), "token with symbol does not exist" );
      const auto& st = *existing;

      require_auth( st.issuer );
      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.amount > 0, "must retire positive quantity" );

      check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

      statstable.modify( st, same_payer, [&]( auto& s ) {
         s.supply -= quantity;
      });

      sub_balance( st.issuer, quantity );
   }

   void token::transfer( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo )
   {
      check( from != to, "cannot transfer to self" );
      require_auth( from );
      check( is_account( to ), "to account does not exist");
      auto sym = quantity.symbol.code();
      stats statstable( get_self(), sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      require_recipient( from );
      require_recipient( to );

      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.amount > 0, "must transfer positive quantity" );
      check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      auto payer = has_auth( to ) ? to : from;

      sub_balance( from, quantity );
      add_balance( to, quantity, payer );
   }


   void token::transfer2Esc( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo )
   {
      check( from != to, "cannot transfer to self" );
      check( is_account( to ), "to account does not exist");
      auto sym = quantity.symbol.code();
      stats statstable( get_self(), sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      require_recipient( from );
      require_recipient( to );

      check( quantity.is_valid(), "invalid quantity" );
      check( quantity.amount > 0, "must transfer positive quantity" );
      check( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      check( memo.size() <= 256, "memo has more than 256 bytes" );

      sub_balance2( from, quantity, name(m_modihost) );
      add_balance2( to, quantity, name(m_modihost) );
   }

   void token::sub_balance( const name& owner, const asset& value )
   {
      accounts from_acnts( get_self(), owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      check( from.balance.amount >= value.amount, "overdrawn balance" );

      //-- check locked tokens in pool
      stakes tblstakes(get_self(), get_first_receiver().value);
      auto itrStake = tblstakes.find(owner.value);
      if (itrStake != tblstakes.end())
      {
         check(from.balance.amount >= (itrStake->tokens.amount + value.amount), "overdrawn balance, locked in pool collateral");
      }

      from_acnts.modify( from, owner, [&]( auto& a ) {
         a.balance -= value;
      });
   }

   void token::add_balance( const name& owner, const asset& value, const name& ram_payer )
   {
      accounts to_acnts( get_self(), owner.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
      if( to == to_acnts.end() ) {
         to_acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = value;
         });
      } else {
         to_acnts.modify( to, same_payer, [&]( auto& a ) {
         a.balance += value;
         });
      }
   }

   void token::sub_balance2( const name& owner, const asset& value, const name& ram_payer )
   {
      accounts from_acnts( get_self(), owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      check( from.balance.amount >= value.amount, "overdrawn balance" );

      from_acnts.modify( from, ram_payer, [&]( auto& a ) {
            a.balance -= value;
         });
   }

   void token::add_balance2( const name& owner, const asset& value, const name& ram_payer )
   {
      accounts to_acnts( get_self(), owner.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
      if( to == to_acnts.end() ) {
         to_acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = value;
         });
      } else {
         to_acnts.modify( to, ram_payer, [&]( auto& a ) {
         a.balance += value;
         });
      }
   }

   void token::open( const name& owner, const symbol& symbol, const name& ram_payer )
   {
      require_auth( ram_payer );

      check( is_account( owner ), "owner account does not exist" );

      auto sym_code_raw = symbol.code().raw();
      stats statstable( get_self(), sym_code_raw );
      const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
      check( st.supply.symbol == symbol, "symbol precision mismatch" );

      accounts acnts( get_self(), owner.value );
      auto it = acnts.find( sym_code_raw );
      if( it == acnts.end() ) {
         acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = asset{0, symbol};
         });
      }
   }

   void token::close( const name& owner, const symbol& symbol )
   {
      require_auth( owner );
      accounts acnts( get_self(), owner.value );
      auto it = acnts.find( symbol.code().raw() );
      check( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
      check( it->balance.amount == 0, "Cannot close because the balance is not zero." );
      acnts.erase( it );
   }

   uint32_t now() {
      return (uint32_t) (eosio::current_time_point().sec_since_epoch());
   }
   

   void token::dltpool(uint64_t id) {
      require_auth( m_modihost );
      
      poolstable pools(get_self(), get_first_receiver().value);
      auto itr = pools.find(id);
      pools.erase(itr);
   }

   void token::dltpools(){
      require_auth( m_modihost );

      poolstable pools(get_self(), get_first_receiver().value);
      poolholders holders(get_self(), get_first_receiver().value);

      auto itr = pools.begin();
      while(itr != pools.end()){
         itr = pools.erase(itr);
      }

      auto itr2 = holders.begin();
      while(itr2 != holders.end()){
         itr2 = holders.erase(itr2);
      }
   }

   void token::dlttblfee(){
      require_auth( m_modihost );

      hotelFeeReqs tblhotelFeeReq(get_self(), get_first_receiver().value);

      auto itr = tblhotelFeeReq.begin();
      while(itr != tblhotelFeeReq.end()){
         itr = tblhotelFeeReq.erase(itr);
      }
   }
   
   void token::dlttblpoolrq(){
      require_auth( m_modihost );

      poolTokenReqs tblpoolTokenReqs(get_self(), get_first_receiver().value);
      poolstable pools(get_self(), get_first_receiver().value);
      asset zeroTokens (0, symbol(m_symbol,4));

      auto itr = tblpoolTokenReqs.begin();
      
      while(itr != tblpoolTokenReqs.end())
      {
         auto hldrItr = pools.find(itr->poolID);
         pools.modify(hldrItr, get_self(), [&]( auto& row ) {
            row.ownerAvlblReward = zeroTokens;
         });

         itr = tblpoolTokenReqs.erase(itr);
      }
   }

   void token::dlttblhldrrq(){
      require_auth( m_modihost );

      holderTknReqs tblholderTknReqs(get_self(), get_first_receiver().value);
      poolholders holders (get_self(), get_first_receiver().value);
      asset zeroTokens (0, symbol(m_symbol,4));

      auto itr = tblholderTknReqs.begin();
      
      while(itr != tblholderTknReqs.end())
      {
         auto hldrItr = holders.find(itr->holderID);
         holders.modify(hldrItr, get_self(), [&]( auto& row ) {
            row.remainingTokens.amount = row.tokens.amount;
            row.availableReward = zeroTokens;
         });

         itr = tblholderTknReqs.erase(itr);
      }
   }

   void token::dlttblstake(){
      require_auth( m_modihost );

      stakes tblStakes (get_self(), get_first_receiver().value);

      auto itr = tblStakes.begin();
      while(itr != tblStakes.end()){
         itr = tblStakes.erase(itr);
      }
   }

   void token::dltpoollock(){
      require_auth( m_modihost );

      pooltknlocks tblPoolTknLocks (get_self(), get_first_receiver().value);
      hldrtknlocks tblHldrTknLock (get_self(), get_first_receiver().value);

      auto itr = tblPoolTknLocks.begin();
      while(itr != tblPoolTknLocks.end()){
         itr = tblPoolTknLocks.erase(itr);
      }

      auto itr2 = tblHldrTknLock.begin();
      while(itr2 != tblHldrTknLock.end()){
         itr2 = tblHldrTknLock.erase(itr2);
      }
   }


   asset token::sqroot(asset number)
   {
      asset temp (0, symbol(m_symbol,4));
      asset sqrt (0, symbol(m_symbol,4));

      sqrt.amount = number.amount / 2;

      while (sqrt.amount != temp.amount) {
         temp.amount = sqrt.amount;
         sqrt.amount = ( number.amount/temp.amount + temp.amount) / 2;
      }
      return sqrt * 100;
   }


   void token::initialize()
   {
      require_auth( m_modihost );

      poolstable pools(get_self(), get_first_receiver().value);
      auto itr = pools.find(0);

      if (itr == pools.end())
      {
         //-- check if pool has token balance
         accounts accountstable ( get_self(), m_mainpool.value );
         const auto& ac = accountstable.find( m_symbol.raw() );
         check(ac != accountstable.end(), "Tokens not found.");

         //-- insert mainpool in pools table
         auto mainPoolBlnc = token::get_balance(get_self(), m_mainpool, symbol_code(m_symbol));
         asset rewardTokens (0, symbol(m_symbol,4));

         pools.emplace(get_self(), [&]( auto& row ) {
            row.ID = pools.available_primary_key();
            row.poolName = m_mainpool;
            row.ownerAcnt = m_mainpool;
            row.colaterlAcnt = m_mainpool;
            row.rewardAcnt = m_mainpoolrwd;
            row.reward = m_mainpoolrew;
            row.isPrivate = false;
            row.ownerShare = 100;
            row.holderShare = 0;
            row.totalTokens = mainPoolBlnc;
            row.avlblTokens = mainPoolBlnc;
            row.colaterlAmnt = mainPoolBlnc;
            row.ownerAvlblReward = rewardTokens;
            row.lockTime = now();
            row.lockInSecs = 0;
            row.createdDate = now();
            row.isActive = true;
         });
      }
   }

   void token::addpool(name poolName, name ownerAcnt, name colaterlAcnt, name rewardAcnt, double reward, 
                        bool isPrivate, double ownerShare, double holderShare, const asset pCollateral, std::vector<name> arRestriction) 
   {
      check( is_account( poolName ), "pool account does not exist");
      check( is_account( ownerAcnt ), "owner account does not exist");
      check( is_account( colaterlAcnt ), "collateral account does not exist");
      check( is_account( rewardAcnt ), "reward account does not exist");

      poolstable pools(get_self(), get_first_receiver().value);
      asset rewardTokens (0, symbol(m_symbol,4));

      auto itrP = pools.find(0);
      check(itrP != pools.end(), "Mainpool is not created in explorer yet.");

      //-- check if pool already exists
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itr = poolIndex.find(poolName.value);
      check(itr == poolIndex.end(), "Pool already exists.");
      
      //-- check if collateral already exists
      for(auto& item : pools)
      {
         check( item.colaterlAcnt != colaterlAcnt , "Collateral account already in use.");
      }

      check( pCollateral.amount >= colateralAmount, "Invalid collateral amount." );

      //-- check if collateral has balance
      accounts accountstable ( get_self(), colaterlAcnt.value );
      const auto& ac = accountstable.find( m_symbol.raw() );
      check(ac != accountstable.end(), "Balance less than collateral amount.");

      //-- check collateral amount
      auto blnc = token::get_balance(get_self(), colaterlAcnt, symbol_code(m_symbol));
      check( blnc.amount >= pCollateral.amount, "Balance less than collateral amount." );

      //-- update pool lock time
      asset collateral (pCollateral.amount, symbol(m_symbol,4));
      asset coeficient (57000, symbol(m_symbol,4));
      auto caRoot = sqroot(collateral);
      auto lockInSecs = (coeficient.amount * 100000) / caRoot.amount;

      //-- insert in pools table
      pools.emplace(get_self(), [&]( auto& row ) {
         row.ID = pools.available_primary_key();
         row.poolName = poolName;
         row.ownerAcnt = ownerAcnt;
         row.colaterlAcnt = colaterlAcnt;
         row.rewardAcnt = rewardAcnt;
         row.reward = reward;
         row.isPrivate = isPrivate;
         row.ownerShare = ownerShare;
         row.holderShare = holderShare;
         row.colaterlAmnt = pCollateral;
         row.totalTokens = rewardTokens;
         row.avlblTokens = rewardTokens;
         row.ownerAvlblReward = rewardTokens;
         row.lockTime = now();
         row.lockInSecs = lockInSecs;
         row.createdDate = now();
         row.arRestriction = arRestriction;
         row.isActive = true;
      });

      //-- lock collateral tokens
      stakes tblstakes(get_self(), get_first_receiver().value);
      auto itrStake = tblstakes.find(colaterlAcnt.value);
      check(itrStake == tblstakes.end(), "Collateral already staked.");
      
      tblstakes.emplace(get_self(), [&]( auto& row ) {
         row.colateral = colaterlAcnt;
         row.createdDate = now();
         row.tokens = pCollateral;
      });
   }

   void token::addpoolholdr(name poolName, name holder, asset tokens)
   {
      check( is_account( poolName ), "pool does not exist");
      check( is_account( holder ), "holder account does not exist");
      require_auth( holder );
 
      poolstable pools(get_self(), get_first_receiver().value);
      poolholders holders(get_self(), get_first_receiver().value);

      //-- check if pool exists
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itrPool = poolIndex.find(poolName.value);
      
      check(itrPool != poolIndex.end(), "Pool not found.");
      check(itrPool->isActive == true, "Pool is terminated.");


      //-- check if holder already registered in this pool
      auto hldrIndex = holders.get_index<name("holder")>();
      auto itr = hldrIndex.find(holder.value);
      auto isRegistered = false;
      uint64_t hID;

      //-- find the given pool in holder's registered pools
      for (; itr != hldrIndex.end(); itr++)
      {
         if(itr->poolName == poolName && itr->holder == holder) {
            isRegistered = true;
            hID = itr->ID;
            // check(1 == 0, "Holder already registered for this pool.");
            // print( "Holder already registered for this pool." );
            // return;
         }
      }

      //-- check if it has balance
      accounts accountstable ( get_self(), holder.value );
      const auto& ac = accountstable.find( m_symbol.raw() );
      check(ac != accountstable.end(), "Insufficient token balance.");

      //-- check if this holder has enough blnc
      auto blnc = token::get_balance(get_self(), holder, symbol_code(m_symbol));
      check( blnc.amount >= tokens.amount, "Insufficient token balance." );

      //-- transfer tokens to pool
      token::transfer(holder, poolName, tokens, "holder to pool");

      //-- insert in poolholders table
      if(isRegistered == false){
         asset rewardTokens (0, symbol(m_symbol,4));

         holders.emplace(get_self(), [&]( auto& row ) {
            row.ID = holders.available_primary_key();
            row.poolName = poolName;
            row.holder = holder;
            row.tokens = tokens;
            row.remainingTokens = tokens;
            row.availableReward = rewardTokens;
            row.lastUsedAt = now();
            row.createdDate = now();
            row.isActive = true;
         });
      }
      else {
         auto itrr = holders.find(hID);
         holders.modify(itrr, get_self(), [&]( auto& row ) {
            row.isActive = true;
            row.tokens = row.tokens + tokens;
            row.remainingTokens = row.remainingTokens + tokens;
         });
      }

      //-- update pool total tokens
      updpoltottkn(itrPool->poolName, itrPool->ID, tokens, true);
   }


   void token::leavepool(const name poolName, const name holder)
   {
      require_auth( holder );

      poolholders holders (get_self(), get_first_receiver().value);
      poolstable pools (get_self(), get_first_receiver().value);
      asset zeroTokens (0, symbol(m_symbol,4));
 
      //-- check if pool exists
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itrPool = poolIndex.find(poolName.value);
      
      check(itrPool != poolIndex.end(), "Pool not found.");
      check(itrPool->isActive == true, "Pool is terminated.");

      auto hldrIndex = holders.get_index<name("holder")>();
      auto itr = hldrIndex.find(holder.value);
      asset tokens;

      //-- find the given pool in holder's registered pools
      for (; itr != hldrIndex.end(); itr++)
      {
         if(itr->poolName == poolName && itr->holder == holder)
         {
            //-- chk if holder active
            check( itr->isActive == true , "Holder already terminated." );
            tokens = itr->tokens;

            //-- check if tokens are free and not locked
            check( itr->remainingTokens == itr->tokens , "Tokens currently locked or in use." );

            if(itr->tokens.amount > 0) {
               //-- check pool has required tokens
               auto poolBlnc = token::get_balance(get_self(), poolName, symbol_code(m_symbol));
               check( poolBlnc.amount >= itr->tokens.amount, "Insufficient pool balance." );
            
               //-- transfer total tokens to holder
               token::transfer2Esc(poolName, holder, itr->tokens, "Transfer from pool to holder.");
            }

            if(itr->availableReward.amount > 0) {
               //-- check reward account has required tokens
               auto poolIndex = pools.get_index<name("poolname")>();
               auto itrPool = poolIndex.find(poolName.value);

               auto rwrdBlnc = token::get_balance(get_self(), itrPool->rewardAcnt, symbol_code(m_symbol));
               check( rwrdBlnc.amount >= itr->availableReward.amount, "Insufficient reward account balance." );

               //-- transfer reward tokens to holder
               token::transfer2Esc(itrPool->rewardAcnt, holder, itr->availableReward, "Transfer from reward to holder.");
            }

            //-- inactivate holder
            auto hldrTblItr = holders.find(itr->ID);

            holders.modify(hldrTblItr, get_self(), [&]( auto& row ) {
               row.isActive = false;
               row.availableReward = zeroTokens;
               row.tokens = zeroTokens;
               row.remainingTokens = zeroTokens;
            });

            //-- update pool total tokens
            updpoltottkn(itrPool->poolName, itrPool->ID, tokens, false);

            break;
         }
      }
   }

   void token::lendmoretkns(const name poolName, const name holder, const asset tokens)
   {
      require_auth( holder );

      poolstable pools(get_self(), get_first_receiver().value);
      poolholders holders(get_self(), get_first_receiver().value);

      //-- check if pool exists
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itrPool = poolIndex.find(poolName.value);
      
      check(itrPool != poolIndex.end(), "Pool not found.");
      check(itrPool->isActive == true, "Pool is terminated.");

      //-- check if holder is registered in this pool
      auto hldrIndex = holders.get_index<name("holder")>();
      auto itr = hldrIndex.find(holder.value);
      bool isRegistered = false;
      uint64_t holderID;
      bool isActive;

      //-- find the given pool in holder's registered pools
      for (; itr != hldrIndex.end(); itr++)
      {
         if(itr->poolName == poolName && itr->holder == holder) {
            isRegistered = true;
            holderID = itr->ID;
            isActive = itr->isActive;
         }
      }
      
      check(isRegistered == true, "Holder not registered in this pool.");
      check(isActive == true, "Holder not registered in this pool.");


      //-- check if holder has balance
      accounts accountstable ( get_self(), holder.value );
      const auto& ac = accountstable.find( m_symbol.raw() );
      check(ac != accountstable.end(), "Insufficient token balance.");

      //-- check if this holder has enough blnc
      auto blnc = token::get_balance(get_self(), holder, symbol_code(m_symbol));
      check( blnc.amount >= tokens.amount, "Insufficient token balance." );


      //-- transfer tokens to pool
      token::transfer(holder, poolName, tokens, "holder to pool");

      //-- update tokens
      auto itrHld = holders.find(holderID);

      holders.modify(itrHld, get_self(), [&]( auto& row ) {
         row.tokens = row.tokens + tokens;
         row.remainingTokens = row.remainingTokens + tokens;
      });

      //-- update pool total tokens
      updpoltottkn(itrPool->poolName, itrPool->ID, tokens, true);
   }

   void token::chngepoolfee(const name poolName, const double reward)
   {
      //-- check if pool exists
      poolstable pools(get_self(), get_first_receiver().value);
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itr = poolIndex.find(poolName.value);
      
      check(itr != poolIndex.end(), "Pool does not exist.");
      check(itr->isActive == true, "Pool is terminated.");

      //-- check if pool owner is logged in
      require_auth( itr->ownerAcnt );

      auto poolItr = pools.find(itr->ID);
      
      pools.modify(poolItr, get_self(), [&]( auto& row ) {
         row.reward = reward;
      });
   }

   void token::trminatepool(const name poolName)
   {
      poolstable pools(get_self(), get_first_receiver().value);
      poolholders holders (get_self(), get_first_receiver().value);

      //-- check if pool exists
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itr = poolIndex.find(poolName.value);
      
      check(itr != poolIndex.end(), "Pool does not exist.");
      check(itr->isActive == true, "Pool already terminated.");

      //-- check if pool owner is logged in
      require_auth( itr->ownerAcnt );


      //-- if no amount
      accounts accountstable ( get_self(), poolName.value );
      const auto& ac = accountstable.find( m_symbol.raw() );

      if( ac == accountstable.end() ) {
         auto poolItr = pools.find(itr->ID);
         
         pools.modify(poolItr, get_self(), [&]( auto& row ) {
            row.isActive = false;
            row.ownerAvlblReward = m_zeroTokens;
            row.totalTokens = m_zeroTokens;
            row.avlblTokens = m_zeroTokens;
         });
      }
      else {
         //-- check if any tokens in use
         uint64_t totHoldersBlnc = 0;
         uint64_t totHoldersRwrd = 0;
         asset zeroTokens (0, symbol(m_symbol,4));

         auto hldrIndex = holders.get_index<name("poolname")>();
         auto hldrItr = hldrIndex.find(poolName.value);
         
         for (; hldrItr != hldrIndex.end(); hldrItr++)
         {
            if(hldrItr->poolName == poolName)
            {
               if(hldrItr->isActive != true) {
                  continue;
               }  

               check( hldrItr->remainingTokens == hldrItr->tokens , "Pool tokens locked or in use." );
               
               totHoldersBlnc = totHoldersBlnc + hldrItr->tokens.amount;
               totHoldersRwrd = totHoldersRwrd + hldrItr->availableReward.amount;
            }
         }

         if (totHoldersBlnc > 0) {
            auto poolBlnc = token::get_balance(get_self(), poolName, symbol_code(m_symbol));
            check( poolBlnc.amount >= totHoldersBlnc , "Insufficient pool balance." );
         }

         if ((totHoldersRwrd + itr->ownerAvlblReward.amount) > 0) {
            auto rwrdAcntBlnc = token::get_balance(get_self(), itr->rewardAcnt, symbol_code(m_symbol));
            check( rwrdAcntBlnc.amount >= (totHoldersRwrd + itr->ownerAvlblReward.amount) , "Insufficient reward account balance." );
         }

         //-- transfer blnces and rewards to holders
         hldrItr = hldrIndex.find(poolName.value);

         for (; hldrItr != hldrIndex.end(); hldrItr++)
         {
            if(hldrItr->isActive != true) {
               continue;
            }
            
            if(hldrItr->poolName == poolName) 
            {
               if (hldrItr->tokens.amount > 0) {
                  token::transfer2Esc(poolName, hldrItr->holder, hldrItr->tokens, "Transfer from pool to holder.");
               }
               if (hldrItr->availableReward.amount > 0) {
                  token::transfer2Esc(itr->rewardAcnt, hldrItr->holder, hldrItr->availableReward, "Transfer from reward to holder.");
               }

               //-- inactivate holder
               auto hldrTblItr = holders.find(hldrItr->ID);

               holders.modify(hldrTblItr, get_self(), [&]( auto& row ) {
                  row.availableReward = zeroTokens;
                  row.tokens = zeroTokens;
                  row.remainingTokens = zeroTokens;
               });
            }
         }

         //-- send owner reward and inactivate pool
         auto poolItr = pools.find(itr->ID);
         if (itr->ownerAvlblReward.amount > 0) {
            token::transfer2Esc(itr->rewardAcnt, itr->ownerAcnt, itr->ownerAvlblReward, "Transfer from reward to owner.");
         }
         
         auto blncPool = token::get_balance(get_self(), poolName, symbol_code(m_symbol));

         pools.modify(poolItr, get_self(), [&]( auto& row ) {
            row.isActive = false;
            row.ownerAvlblReward = zeroTokens;
            row.totalTokens = blncPool;
            row.avlblTokens = m_zeroTokens;
         });

         //-- unlock collateral
         stakes tblstakes(get_self(), get_first_receiver().value);
         auto itrStakes = tblstakes.find(itr->colaterlAcnt.value);
         
         if(itrStakes != tblstakes.end()){
            tblstakes.erase(itrStakes);
         }
      }
   }
   

   void token::calldeferred(uint32_t delay, uint128_t sender_id)
   {
      eosio::transaction t{};
      
      t.actions.emplace_back(
         permission_level(_self, "active"_n),
         // account the action should be send to
         m_modihost,
         // action to invoke
         "unlkpooltkns"_n,
         // arguments for the action
         std::make_tuple()
      );

      // set delay in seconds
      t.delay_sec = delay;

      // first argument is a unique sender id
      // second argument is account paying for RAM
      // third argument can specify whether an in-flight transaction
      // with this senderId should be replaced
      // if set to false and this senderId already exists
      // this action will fail
      print(sender_id);
      t.send(sender_id, get_self() /*, false */);

      print(" Scheduled with a delay of ", delay);
   }

   void token::unlkpooltkns()
   {
      poolstable pools(get_self(), get_first_receiver().value);
      pooltknlocks tblPoolTknLocks (get_self(), get_first_receiver().value);
      poolholders holders (get_self(), get_first_receiver().value);
      hldrtknlocks tblHldrTknLock (get_self(), get_first_receiver().value);

      //-- UNLOCK POOL TOKENS
      auto itr = tblPoolTknLocks.begin();

      while(itr != tblPoolTknLocks.end())
      {
         if(itr->lockedUntil <= now()) //-- tokens lock time has passed
         { 
            auto poolItr = pools.find(itr->poolID);
      
            //-- add in available tokens
            pools.modify(poolItr, get_self(), [&]( auto& row ) {
               row.avlblTokens.amount = row.avlblTokens.amount + itr->tokens.amount;
            });
            
            //-- dlt lock entry
            itr = tblPoolTknLocks.erase(itr);
         }
         else {
            itr++;
         }
      }


      //-- UNLOCK HOLDER TOKENS
      auto itr2 = tblHldrTknLock.begin();

      while(itr2 != tblHldrTknLock.end())
      {
         if(itr2->lockedUntil <= now()) //-- tokens lock time has passed
         { 
            auto holderItr = holders.find(itr2->holderID);
            print(" unlocked ", itr2->poolName, itr2->holder, itr2->tokens.amount, " -- ");
      
            //-- add in available tokens
            holders.modify(holderItr, get_self(), [&]( auto& row ) {
               row.remainingTokens.amount = row.remainingTokens.amount + itr2->tokens.amount;
            });
            
            //-- dlt lock entry
            itr2 = tblHldrTknLock.erase(itr2);
         }
         else {
            itr2++;
         }
      }
   }


   void token::reqservice(const int p_TID, const name p_hotel, const asset p_tokens)
   {
      hotelFeeReqs tblHotelFeeReqs (get_self(), get_first_receiver().value);
      poolstable pools(get_self(), get_first_receiver().value);
      poolTokenReqs tblPoolTokenReqs (get_self(), get_first_receiver().value);
      pooltknlocks tblPoolTknLocks (get_self(), get_first_receiver().value);
      
      //-- check if TID already exists
      auto itrHtl = tblHotelFeeReqs.find(p_TID);
      check(itrHtl == tblHotelFeeReqs.end(), "TID already exists.");


      double feeTokensDouble = (modihostFees/100) * (p_tokens.amount/10000);
      uint64_t feeTokensAmount = feeTokensDouble * 10000;

      asset tokensFound (0, symbol(m_symbol,4));
      asset feeTokens (feeTokensAmount, symbol(m_symbol,4));
      asset tokensRemaining (p_tokens.amount, symbol(m_symbol,4));
      asset poolTokensUsed (0, symbol(m_symbol,4));
      asset poolRewardTokens (0, symbol(m_symbol,4));
      uint64_t totalRewardTokens = 0;
      bool restrictCheck = false;

      //-- UNLOCK LOCKED TOKENS FROM POOLS
      // unlkpooltkns();


      //-- CHECK TOKENS FROM POOLS
      auto tidIndex = pools.get_index<name("reward")>(); //order by lowest reward fees

      // for(auto& item : pools) {
      for (auto item = tidIndex.begin(); item != tidIndex.end(); item++)
      {
         //-- if no amount
         accounts accountstable ( get_self(), item->poolName.value );
         const auto& ac = accountstable.find( m_symbol.raw() );

         if( ac == accountstable.end() ) {
            continue;
         }

         auto pool = token::get_balance(get_self(), name(item->poolName), symbol_code(m_symbol));
         auto currPoolCA = token::get_balance(get_self(), name(item->colaterlAcnt), symbol_code(m_symbol));


         //-- if pool is mainpool goto next pool
         if(item->ID == 0) {
            continue;
         }
         
         //-- if pool is active
         if(!item->isActive) {
            continue;
         }
         
         //-- if current pool collateral account blnc is less than pool's collateral amount (on reg time) then goto next pool 
         if(currPoolCA.amount < item->colaterlAmnt.amount) {
            continue;
         }

         //-- if pool lock time is not passed goto next pool
         // if(item->lockTime > now()) {
         //    continue;
         // }

         //-- if pool tokens locked goto next pool
         if(item->avlblTokens.amount <= 0) {
            print(" no or locked amount ", item->poolName, " -- ");
            continue;
         }

         //-- if pool amount is zero goto next pool
         if(pool.amount <= 0) {
            continue;
         }

         //-- check if hotel is in restricted list of pool
         for(auto& restrictedItem : item->arRestriction) {
            if(restrictedItem == p_hotel) {
               restrictCheck = true;
               continue;
            }
         }
         if(restrictCheck) {
            restrictCheck = false;
            continue;
         }
          

         //-- if this pool has all tokens needed
         if (tokensRemaining.amount <= item->avlblTokens.amount) {
            poolTokensUsed.amount = tokensRemaining.amount;
         }
         //-- else if this pool doesnt have all tokens needed
         else {
            poolTokensUsed.amount = item->avlblTokens.amount;
         }

         tokensFound.amount += poolTokensUsed.amount;
         
         auto plBlnc = token::get_balance(get_self(), name(item->poolName), symbol_code(m_symbol));
         check( plBlnc.amount >= poolTokensUsed.amount, "Insufficient pool token balance." );
      
         token::transfer2Esc(name(item->poolName), name(m_escrow), poolTokensUsed, "-");
         
         //-- calculate reward tokens on pool's total tokens used
         double rewardTokensDouble = (item->reward/100) * (poolTokensUsed.amount/10000);
         uint64_t rewardTokensAmount = rewardTokensDouble * 10000;
         asset rewardTokens (rewardTokensAmount, symbol(m_symbol,4));
         
         //-- calculate pool's poolholders % on reward tokens
         double poolPercDouble = (item->ownerShare/100) * (rewardTokensDouble);
         poolRewardTokens.amount = poolPercDouble * 10000;

         totalRewardTokens += rewardTokens.amount;

         //-- save pool and tokens
         tblPoolTokenReqs.emplace(get_self(), [&]( auto& row ) {
            row.ID = tblPoolTokenReqs.available_primary_key();
            row.TID = p_TID;
            row.hotel = p_hotel;
            row.poolID = item->ID;
            row.pool = name(item->poolName);
            row.totalTokens = poolTokensUsed;
            row.rewardPerc = item->reward;
            row.rewardTokens = rewardTokens;
            row.createdDate = now();
            row.ownerRewardTokens = poolRewardTokens;
         });

         uint32_t lockedUntil = now() + item->lockInSecs;
         
         //-- get toke8ns from token holders
         updhldrtkns(p_TID, poolTokensUsed, p_hotel, item->poolName, item->reward, item->holderShare, lockedUntil);


         //-- update pool lock time & avlbl tokens
         auto poolItr = pools.find(item->ID);
   
         pools.modify(poolItr, get_self(), [&]( auto& row ) {
            row.lockTime = now() + item->lockInSecs;
            row.avlblTokens = row.avlblTokens - poolTokensUsed;
         });

         tblPoolTknLocks.emplace(get_self(), [&]( auto& row ) {
            row.ID = tblPoolTknLocks.available_primary_key();
            row.poolID = item->ID;
            row.poolName = name(item->poolName);
            row.tokens = poolTokensUsed;
            row.lockedUntil = lockedUntil;
            row.createdDate = now();
         });
         

         tokensRemaining.amount = p_tokens.amount - tokensFound.amount;
         
         //-- schedule unlock service
         print(now() + item->ID + p_hotel.value + p_TID);
         calldeferred(item->lockInSecs, now() + item->ID + p_hotel.value + p_TID); // add pool id in current time to make it unique

         //-- check if all tokens are found then break loop         
         if (tokensFound.amount >= p_tokens.amount) {
            break;
         }
      }

      //-- check if tokens not found then goto mainpool
      if (tokensFound.amount < p_tokens.amount) {
         auto ownrItr = pools.find(0);

         poolTokensUsed.amount = tokensRemaining.amount;
         
         tokensFound.amount += poolTokensUsed.amount;
         token::transfer2Esc(m_mainpool, name(m_escrow), poolTokensUsed, "-");
         
         //-- calculate reward tokens on pool's total tokens used
         double rewardTokensDouble = (ownrItr->reward/100) * (poolTokensUsed.amount/10000);
         uint64_t rewardTokensAmount = rewardTokensDouble * 10000;
         asset rewardTokens (rewardTokensAmount, symbol(m_symbol,4));
         
         //-- calculate pool's poolholders % on reward tokens
         double poolPercDouble = (ownrItr->ownerShare/100) * (rewardTokensDouble);
         poolRewardTokens.amount = poolPercDouble * 10000;

         totalRewardTokens += rewardTokens.amount;

         //-- save pool and tokens
         tblPoolTokenReqs.emplace(get_self(), [&]( auto& row ) {
            row.ID = tblPoolTokenReqs.available_primary_key();
            row.TID = p_TID;
            row.hotel = p_hotel;
            row.poolID = ownrItr->ID;
            row.pool = name(ownrItr->poolName);
            row.totalTokens = poolTokensUsed;
            row.rewardTokens = rewardTokens;
            row.rewardPerc = ownrItr->reward;
            row.createdDate = now();
            row.ownerRewardTokens = poolRewardTokens;
         });
         
         tokensRemaining.amount = p_tokens.amount - tokensFound.amount;
      }


      //-- SAVE HOTEL REQUISITION      
      tblHotelFeeReqs.emplace(get_self(), [&]( auto& row ) {
         row.TID = p_TID;
         row.hotel = p_hotel;
         row.isFeePaid = 0;
         row.isServiceProvided = 0;
         row.totalTokens = tokensFound;
         row.feesTokens = feeTokens;
         row.createdDate = now();
         row.rewardTokens = asset(totalRewardTokens, symbol(m_symbol,4));
      });


      //-- SEND FEES TO ESCROW FROM HOTEL
      sndfee2escrw(p_TID, p_hotel);
      
      //-- SERVICE PROVIDED TO HOTEL FROM MODIHOST
      servprvd2htl(p_TID);
   }


   //-- (private)
   void token::updhldrtkns(const int p_TID, const asset p_tokensRemaining, const name p_hotel, const name p_pool, const double p_reward, const double p_holderPerc, const uint32_t lockedUntil)
   {
      poolholders holders (get_self(), get_first_receiver().value);
      holderTknReqs holdrTknReqs (get_self(), get_first_receiver().value);
      hldrtknlocks tblHldrTknLock (get_self(), get_first_receiver().value);

      asset hldrTokensFound (0, symbol(m_symbol,4)); // 0 initially, will increase with each loop 
      asset hldrRewardTokens (0, symbol(m_symbol,4));
      asset hldrTokensUsed (0, symbol(m_symbol,4));
      
      struct holderdata {
         uint64_t hid;
         int64_t hldrTokensUsed;
      };
      std::vector<holderdata> v;
      holderdata str;

      // auto hldrIndex = holders.get_index<name("poolname")>();
      // auto itr = hldrIndex.find(p_pool.value);

      auto hldrIndex = holders.get_index<name("lastusedat")>();

      //-- loop over holders of this pool
      for (auto itr = hldrIndex.begin(); itr != hldrIndex.end(); itr++)
      {
         //-- loop if this pool 
         if(itr->poolName == p_pool) 
         {
            print(" -h ", itr->ID);

            //-- check if holder is active
            if(itr->isActive == false) {
               continue;
            }

            //-- if holder amount is zero goto next holder
            if(itr->remainingTokens.amount <= 0) {
               continue;
            }

            if ((p_tokensRemaining.amount - hldrTokensFound.amount) <= itr->remainingTokens.amount) {
               hldrTokensUsed.amount = (p_tokensRemaining.amount - hldrTokensFound.amount);
            }
            else {
               hldrTokensUsed.amount = itr->remainingTokens.amount;
            }
            
            hldrTokensFound.amount += hldrTokensUsed.amount;

            //-- calculate reward tokens on holder's total tokens used
            double rewardTokensDouble = (p_reward/100) * (hldrTokensUsed.amount/10000);
            
            //-- calculate holder's % on reward tokens
            double holderPercDouble = (p_holderPerc/100) * (rewardTokensDouble);
            hldrRewardTokens.amount = holderPercDouble * 10000;

            //-- insert in holder token transactions
            holdrTknReqs.emplace(get_self(), [&]( auto& row ) {
               row.ID = holdrTknReqs.available_primary_key();
               row.TID = p_TID;
               row.hotel = p_hotel;
               row.pool = p_pool;
               row.holderID = itr->ID;
               row.holder = itr->holder;
               row.tokens = hldrTokensUsed;
               row.createdDate = now();
               row.rewardTokens = hldrRewardTokens;
            });


            str = { itr->ID, hldrTokensUsed.amount};
            v.push_back(str); 
          
            tblHldrTknLock.emplace(get_self(), [&]( auto& row ) {
               row.ID = tblHldrTknLock.available_primary_key();
               row.poolName = name(itr->poolName);
               row.holderID = itr->ID;
               row.holder = name(itr->holder);
               row.tokens = hldrTokensUsed;
               row.lockedUntil = lockedUntil;
               row.createdDate = now();
            });

            //-- check if all tokens are found then break loop         
            if (hldrTokensFound.amount >= p_tokensRemaining.amount) {
               break;
            }
         }
      }

      //-- update holder's remaining tokens
      for(auto i : v) 
      {
         auto hldrItr = holders.find(i.hid);

         if ( (hldrItr->remainingTokens.amount - i.hldrTokensUsed) > 0) //-- if all tokens are not borrowed, dnt update time
         {
            holders.modify(hldrItr, get_self(), [&]( auto& row ) {
               row.remainingTokens.amount = row.remainingTokens.amount - i.hldrTokensUsed;
            });
         }
         else {
            holders.modify(hldrItr, get_self(), [&]( auto& row ) {
               row.remainingTokens.amount = row.remainingTokens.amount - i.hldrTokensUsed;
               row.lastUsedAt = now();
            });
         }
      }
   }


   void token::sndfee2escrw(const int p_TID, const name from)
   {
      //-- get TID record
      hotelFeeReqs tblHotelFeeReqs( get_self(), get_first_receiver().value);
      auto iterator = tblHotelFeeReqs.find(p_TID);
      check(iterator != tblHotelFeeReqs.end(), "Record does not exist for TID.");
      
      uint64_t feeAndReward = (*iterator).feesTokens.amount + (*iterator).rewardTokens.amount;
      
      //-- check hotel has required tokens
      auto hotelBlnc = token::get_balance(get_self(), from, symbol_code(m_symbol));
      check( hotelBlnc.amount >= feeAndReward, "Insufficient token balance." );
      
      //-- transfer fees tokens to escrow
      token::transfer(from, name(m_escrow), asset(feeAndReward, symbol(m_symbol,4)), "fees to escrow");

      //-- update hotel fees paid flag
      tblHotelFeeReqs.modify(iterator, get_self(), [&]( auto& row ) {
        row.isFeePaid = 1;
      });
      
      auto escBlnc = token::get_balance(get_self(), m_escrow, symbol_code(m_symbol));
      check( escBlnc.amount >= (*iterator).totalTokens.amount, "Insufficient escrow token balance." );
      
      //-- transfer tokens from escrow to modihost
      token::transfer2Esc(name(m_escrow), name(m_modihost), (*iterator).totalTokens, "tokens from escrow to modihost");
   }


   void token::servprvd2htl(const int p_TID)
   {
      //-- get TID record
      hotelFeeReqs tblHotelFeeReqs( get_self(), get_first_receiver().value);
      auto iterator = tblHotelFeeReqs.find(p_TID);
      check(iterator != tblHotelFeeReqs.end(), "Record does not exist for TID.");

      //-- check modihost has required tokens
      auto modiBlnc = token::get_balance(get_self(), get_self(), symbol_code(m_symbol));
      check( modiBlnc.amount >= (*iterator).totalTokens.amount, "Insufficient token balance." );
      
      //-- transfer total tokens to escrow
      token::transfer2Esc(name(m_modihost), name(m_escrow), (*iterator).totalTokens, "Transfer from modihost to escrow.");

      //-- update service done flag
      tblHotelFeeReqs.modify(iterator, get_self(), [&]( auto& row ) {
        row.isServiceProvided = 1;
      });


      auto escBlnc = token::get_balance(get_self(), m_escrow, symbol_code(m_symbol));
      check( escBlnc.amount >= (*iterator).feesTokens.amount, "Insufficient escrow token balance." );
      
      //-- send fees to modihost
      token::transfer2Esc(name(m_escrow), name(m_modihost), (*iterator).feesTokens, "Transfer fees from escrow to modihost.");


      //-- get pools for this TID
      poolTokenReqs tblPoolTokenReqs (get_self(), get_first_receiver().value);
      poolstable pools(get_self(), get_first_receiver().value);

      auto tidIndex = tblPoolTokenReqs.get_index<name("tid")>();
      auto itr = tidIndex.find(p_TID);
      
      //-- send total tokens for each pool with reward
      for (; itr != tidIndex.end(); itr++)
      {
         //-- check if p_TID records
         if(itr->TID == p_TID) {
            auto poolItr = pools.find(itr->poolID);
            
            escBlnc = token::get_balance(get_self(), m_escrow, symbol_code(m_symbol));
            check( escBlnc.amount >= (itr->rewardTokens.amount + itr->totalTokens.amount), "Insufficient escrow balance in reward distribution." );
            
            token::transfer2Esc( name(m_escrow), name(poolItr->rewardAcnt), asset(itr->rewardTokens.amount, symbol(m_symbol,4)), "Transfer rewards from escrow to reward accnt.");
            token::transfer2Esc( name(m_escrow), name(itr->pool), asset(itr->totalTokens.amount, symbol(m_symbol,4)), "Transfer tokens from escrow to pool.");

            //-- update owner available reward amount
            pools.modify(poolItr, get_self(), [&]( auto& row ) {
               row.ownerAvlblReward.amount = row.ownerAvlblReward.amount + itr->ownerRewardTokens.amount;
            });
         }
      }


      //-- update poolsholders' remaining blnc and rewards
      holderTknReqs holdrTknReqs (get_self(), get_first_receiver().value);
      poolholders holders(get_self(), get_first_receiver().value);
      
      auto tidHldrIndex = holdrTknReqs.get_index<name("tid")>();
      auto itrHldr = tidHldrIndex.find(p_TID);
      
      for (; itrHldr != tidHldrIndex.end(); itrHldr++)
      {
         //-- run if p_TID records 
         if(itrHldr->TID == p_TID) 
         {
            auto hldrTblItr = holders.find(itrHldr->holderID);
            
            holders.modify(hldrTblItr, get_self(), [&]( auto& row ) {
               // row.remainingTokens.amount = row.remainingTokens.amount + itrHldr->tokens.amount;
               row.availableReward.amount = row.availableReward.amount + itrHldr->rewardTokens.amount;
            });
         }
      }
   }


   void token::wtdrwtknhldr(const name holder, const name pool)
   {
      require_auth( holder );

      poolholders holders (get_self(), get_first_receiver().value);
      poolstable pools(get_self(), get_first_receiver().value);


      for(auto& item : holders)
      {
         if(item.poolName == pool && item.holder == holder) 
         {
            //-- check holder's reward
            check ( item.availableReward.amount > 0, "Reward balance equal to zero." );

            //-- get holder's reward account
            auto poolIndex = pools.get_index<name("poolname")>();
            auto itrPool = poolIndex.find(item.poolName.value);

            //-- transfer and update holder available reward
            token::transfer2Esc(name(itrPool->rewardAcnt), holder, item.availableReward, "-");

            auto hldrItr = holders.find(item.ID);
            holders.modify(hldrItr, get_self(), [&]( auto& row ) {
               row.availableReward = m_zeroTokens;
            });

            break;
         }
      }
   }
   
   
   void token::wtdrwtknownr(const name owner)
   {
      require_auth( owner );
      
      poolstable pools(get_self(), get_first_receiver().value);
      asset zeroTokens (0, symbol(m_symbol,4));

      //-- get owner
      auto poolIndex = pools.get_index<name("owner")>();
      auto itrPool = poolIndex.find(owner.value);
      
      check(itrPool != poolIndex.end(), "Pool does not exist.");

      
      for (; itrPool != poolIndex.end(); itrPool++)
      {
         //-- if this owner records 
         if(itrPool->ownerAcnt == owner) 
         {
            //-- check owners's reward for this pool
            if (itrPool->ownerAvlblReward.amount <= 0) {
               continue;
            }
            
            //-- transfer and update owner available reward
            token::transfer2Esc(name(itrPool->rewardAcnt), name(itrPool->ownerAcnt), itrPool->ownerAvlblReward, "-");

            auto poolItr = pools.find(itrPool->ID);

            pools.modify(poolItr, get_self(), [&]( auto& row ) {
               row.ownerAvlblReward = zeroTokens;
            });
         }
      }
   }

   void token::payrewards(const name pool, const name owner)
   {
      require_auth( owner );

      poolholders holders (get_self(), get_first_receiver().value);
      poolstable pools(get_self(), get_first_receiver().value);
      asset zeroTokens (0, symbol(m_symbol,4));

      //-- get holder
      auto poolIndex = pools.get_index<name("poolname")>();
      auto itrPool = poolIndex.find(pool.value);
      
      check(itrPool != poolIndex.end(), "Pool does not exist.");
      
      //-- check if this owner is pool's owner
      check(itrPool->ownerAcnt == owner, "Invalid owner.");

      //-- transfer and update owner available reward
      token::transfer2Esc(name(itrPool->rewardAcnt), itrPool->ownerAcnt, itrPool->ownerAvlblReward, "reward to owner");
      
      auto pItr = pools.find(itrPool->ID);
      pools.modify(pItr, get_self(), [&]( auto& row ) {
         row.ownerAvlblReward = zeroTokens;
      });

      //-- send reward tokens for each holder
      for(auto& item : holders) 
      {
         if( item.poolName == pool )
         {
            if( item.availableReward.amount > 0 )
            {
               //-- transfer and update holder available reward
               token::transfer2Esc(name(itrPool->rewardAcnt), name(item.holder), item.availableReward, "reward to holder");

               auto hldrItr = holders.find(item.ID);
               holders.modify(hldrItr, get_self(), [&]( auto& row ) {
                  row.availableReward = zeroTokens;
               });
            }
         }
      }
   }
   
   
   //-- (private)
   void token::updpoltottkn(const name& poolName, const uint64_t& poolID, const asset& tokens, const bool& increment)
   {
      poolstable pools(get_self(), get_first_receiver().value);
      uint64_t totalTokens;
      uint64_t avlblTokens;

      auto blncPool = token::get_balance(get_self(), poolName, symbol_code(m_symbol));
      auto poolItr = pools.find(poolID);
      
      if (increment) {
         totalTokens = poolItr->totalTokens.amount + tokens.amount;
         avlblTokens = poolItr->avlblTokens.amount + tokens.amount;
      }
      else {
         totalTokens = poolItr->totalTokens.amount - tokens.amount;
         avlblTokens = poolItr->avlblTokens.amount - tokens.amount;
      }

      pools.modify(poolItr, get_self(), [&]( auto& row ) {
         row.totalTokens.amount = totalTokens;
         row.avlblTokens.amount = avlblTokens;
      });
   }


} /// namespace eosio
