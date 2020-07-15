#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp> // include this for transactions

#include <cmath>
#include <string>
#include <cstdlib>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   /**
    * @defgroup eosiotoken eosio.token
    * @ingroup eosiocontracts
    *
    * eosio.token contract
    *
    * @details eosio.token contract defines the structures and actions that allow users to create, issue, and manage
    * tokens on eosio based blockchains.
    * @{
    */
   class [[eosio::contract("modihost")]] token : public contract {
      public:
         using contract::contract;

         /**
          * Create action.
          *
          * @details Allows `issuer` account to create a token in supply of `maximum_supply`.
          * @param issuer - the account that creates the token,
          * @param maximum_supply - the maximum supply set for the token created.
          *
          * @pre Token symbol has to be valid,
          * @pre Token symbol must not be already created,
          * @pre maximum_supply has to be smaller than the maximum supply allowed by the system: 1^62 - 1.
          * @pre Maximum supply must be positive;
          *
          * If validation is successful a new entry in statstable for token symbol scope gets created.
          */
         [[eosio::action]]
         void create( const name&   issuer,
                      const asset&  maximum_supply);
         
         /**
          * Issue action.
          *
          * @details This action issues to `to` account a `quantity` of tokens.
          *
          * @param to - the account to issue tokens to, it must be the same as the issuer,
          * @param quntity - the amount of tokens to be issued,
          * @memo - the memo string that accompanies the token issue transaction.
          */
         [[eosio::action]]
         void issue( const name& to, const asset& quantity, const string& memo );

         /**
          * Retire action.
          *
          * @details The opposite for create action, if all validations succeed,
          * it debits the statstable.supply amount.
          *
          * @param quantity - the quantity of tokens to retire,
          * @param memo - the memo string to accompany the transaction.
          */
         [[eosio::action]]
         void retire( const asset& quantity, const string& memo );

         /**
          * Transfer action.
          *
          * @details Allows `from` account to transfer to `to` account the `quantity` tokens.
          * One account is debited and the other is credited with quantity tokens.
          *
          * @param from - the account to transfer from,
          * @param to - the account to be transferred to,
          * @param quantity - the quantity of tokens to be transferred,
          * @param memo - the memo string to accompany the transaction.
          */
         [[eosio::action]]
         void transfer( const name&    from,
                        const name&    to,
                        const asset&   quantity,
                        const string&  memo );
         /**
          * Open action.
          *
          * @details Allows `ram_payer` to create an account `owner` with zero balance for
          * token `symbol` at the expense of `ram_payer`.
          *
          * @param owner - the account to be created,
          * @param symbol - the token to be payed with by `ram_payer`,
          * @param ram_payer - the account that supports the cost of this action.
          *
          * More information can be read [here](https://github.com/EOSIO/eosio.contracts/issues/62)
          * and [here](https://github.com/EOSIO/eosio.contracts/issues/61).
          */
         [[eosio::action]]
         void open( const name& owner, const symbol& symbol, const name& ram_payer );

         /**
          * Close action.
          *
          * @details This action is the opposite for open, it closes the account `owner`
          * for token `symbol`.
          *
          * @param owner - the owner account to execute the close action for,
          * @param symbol - the symbol of the token to execute the close action for.
          *
          * @pre The pair of owner plus symbol has to exist otherwise no action is executed,
          * @pre If the pair of owner plus symbol exists, the balance has to be zero.
          */
         [[eosio::action]]
         void close( const name& owner, const symbol& symbol );

         /**
          * Get supply method.
          *
          * @details Gets the supply for token `sym_code`, created by `token_contract_account` account.
          *
          * @param token_contract_account - the account to get the supply for,
          * @param sym_code - the symbol to get the supply for.
          */
         static asset get_supply( const name& token_contract_account, const symbol_code& sym_code )
         {
            stats statstable( token_contract_account, sym_code.raw() );
            const auto& st = statstable.get( sym_code.raw() );
            return st.supply;
         }

         /**
          * Get balance method.
          *
          * @details Get the balance for a token `sym_code` created by `token_contract_account` account,
          * for account `owner`.
          *
          * @param token_contract_account - the token creator account,
          * @param owner - the account for which the token balance is returned,
          * @param sym_code - the token for which the balance is returned.
          */
         static asset get_balance( const name& token_contract_account, const name& owner, const symbol_code& sym_code )
         {
            accounts accountstable( token_contract_account, owner.value );
            const auto& ac = accountstable.get( sym_code.raw() );
            return ac.balance;
         }

         using create_action = eosio::action_wrapper<"create"_n, &token::create>;
         using issue_action = eosio::action_wrapper<"issue"_n, &token::issue>;
         using retire_action = eosio::action_wrapper<"retire"_n, &token::retire>;
         using transfer_action = eosio::action_wrapper<"transfer"_n, &token::transfer>;
         using open_action = eosio::action_wrapper<"open"_n, &token::open>;
         using close_action = eosio::action_wrapper<"close"_n, &token::close>;



         [[eosio::action]]
         void dlttblfee();

         [[eosio::action]]
         void dlttblpoolrq();

         [[eosio::action]]
         void dlttblhldrrq();

         [[eosio::action]]
         void dlttblstake();

         [[eosio::action]]
         void dltpoollock();

         [[eosio::action]]
         void dltpools();

         [[eosio::action]]
         void dltpool(uint64_t id);


         [[eosio::action]]
         void initialize();

         [[eosio::action]]
         void addpool(name poolName, name ownerAcnt, name colaterlAcnt, name rewardAcnt, double reward, 
                        bool isPrivate, double ownerShare, double holderShare, const asset pCollateral, std::vector<name> arRestriction);
         
         [[eosio::action]]
         void addpoolholdr(name poolName, name holder, asset tokens);
         
         [[eosio::action]]
         void leavepool(const name poolName, const name holder);
         
         [[eosio::action]]
         void lendmoretkns(const name poolName, const name holder, const asset tokens);
         
         [[eosio::action]]
         void chngepoolfee(const name poolName, const double reward);

         [[eosio::action]]
         void trminatepool(const name poolName);

         [[eosio::action]]
         void reqservice(const int p_TID, const name p_hotel, const asset p_tokens);

         [[eosio::action]]
         void sndfee2escrw(const int p_TID, const name from);
         
         [[eosio::action]]
         void servprvd2htl(const int p_TID);

         [[eosio::action]]
         void wtdrwtknhldr(const name holder, const name pool);
         
         [[eosio::action]]
         void wtdrwtknownr(const name owner);

         [[eosio::action]]
         void payrewards(const name pool, const name owner);

         [[eosio::action]]
         void unlkpooltkns();

      //-- END OF PUBLIC REGION


      private:
         const double modihostFees = 0.5; // in %
         const uint64_t colateralAmount = 1000000000;
         const name m_modihost = name("aim");
         const name m_escrow = name("escrow.aim");
         const symbol_code m_symbol = symbol_code("AIM");
         const name m_mainpool = name("mainpool.aim");
         const name m_mainpoolrwd = name("mainpool.aim");
         const double m_mainpoolrew = 0.1;
         const asset m_zeroTokens = asset(0, symbol(m_symbol,4));
         

         struct [[eosio::table]] account {
            asset    balance;

            uint64_t primary_key()const { return balance.symbol.code().raw(); }
         };

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

         struct [[eosio::table]] pool {
            uint64_t ID;
            name poolName;
            name ownerAcnt;
            name colaterlAcnt;
            name rewardAcnt;
            double reward;
            bool isPrivate;
            double ownerShare;
            double holderShare;
            asset totalTokens;
            asset avlblTokens;
            asset colaterlAmnt;
            asset ownerAvlblReward;
            uint32_t lockTime;
            uint32_t lockInSecs;
            uint32_t createdDate;
            bool isActive;
            std::vector<name> arRestriction;
            
            uint64_t primary_key() const { return ID; }
            uint64_t pkpool() const { return poolName.value; }
            uint64_t pkownr() const { return ownerAcnt.value; }
            double pkfee() const { return reward; }
         };

         struct [[eosio::table]] poolholder {
            uint64_t ID;
            name poolName;
            name holder;
            asset tokens;
            asset remainingTokens;
            asset availableReward;
            uint64_t lastUsedAt;
            uint32_t createdDate;
            bool isActive;

            uint64_t primary_key() const { return ID; }
            uint64_t pkholder() const { return holder.value; }
            uint64_t pkhldrpool() const { return poolName.value; }
            uint64_t pklastused() const { return lastUsedAt; }
         };

         struct [[eosio::table]] hotelFeeReq {
            uint64_t TID;
            name hotel;
            uint64_t isFeePaid;
            uint64_t isServiceProvided;
            uint32_t createdDate;
            asset totalTokens;
            asset feesTokens;
            asset rewardTokens;

            uint64_t primary_key() const { return TID; }
         };

         struct [[eosio::table]] poolTokenReq {
            uint64_t ID;
            uint64_t TID;
            name hotel;
            uint64_t poolID;
            name pool;
            asset totalTokens;
            double rewardPerc;
            asset rewardTokens;
            uint32_t createdDate;
            asset ownerRewardTokens;

            uint64_t primary_key() const { return ID; }
            uint64_t pktid() const { return TID; }
            uint64_t pkpool() const { return pool.value; }
         };

         struct [[eosio::table]] holderTknReq {
            uint64_t ID;
            uint64_t TID;
            name hotel;
            name pool;
            uint64_t holderID;
            name holder;
            asset tokens;
            uint32_t createdDate;
            asset rewardTokens;
            
            uint64_t primary_key() const { return ID; }
            uint64_t pktid() const { return TID; }
         };

         struct [[eosio::table]] pooltknlock {
            uint64_t ID;
            uint64_t poolID;
            name poolName;
            asset tokens;
            uint32_t lockedUntil;
            uint32_t createdDate;
            
            uint64_t primary_key() const { return ID; }
         };

         struct [[eosio::table]] hldrtknlock {
            uint64_t ID;
            uint64_t poolID;
            name poolName;
            uint64_t holderID;
            name holder;
            asset tokens;
            uint32_t lockedUntil;
            uint32_t createdDate;
            
            uint64_t primary_key() const { return ID; }
         };

         struct [[eosio::table]] stake {
            name colateral;
            asset tokens;
            uint32_t createdDate;
            
            uint64_t primary_key() const { return colateral.value; }
         };


         typedef eosio::multi_index< "accounts"_n, account > accounts;
         typedef eosio::multi_index< "stat"_n, currency_stats > stats;
         typedef eosio::multi_index< "hotelfeereq"_n, hotelFeeReq > hotelFeeReqs;
         typedef eosio::multi_index< "pooltknlock"_n, pooltknlock > pooltknlocks;
         typedef eosio::multi_index< "hldrtknlock"_n, hldrtknlock > hldrtknlocks;
         typedef eosio::multi_index< "stakes"_n, stake > stakes;

         
         typedef eosio::multi_index< "poolholders"_n, poolholder, 
            eosio::indexed_by< "holder"_n, eosio::const_mem_fun<poolholder, uint64_t, &poolholder::pkholder>>,
            eosio::indexed_by< "poolname"_n, eosio::const_mem_fun<poolholder, uint64_t, &poolholder::pkhldrpool>>,
            eosio::indexed_by< "lastusedat"_n, eosio::const_mem_fun<poolholder, uint64_t, &poolholder::pklastused>>
         > poolholders;

         typedef eosio::multi_index< "pools"_n, pool,
            eosio::indexed_by< "poolname"_n, eosio::const_mem_fun<pool, uint64_t, &pool::pkpool>>,
            eosio::indexed_by< "owner"_n, eosio::const_mem_fun<pool, uint64_t, &pool::pkownr>>,
            eosio::indexed_by< "reward"_n, eosio::const_mem_fun<pool, double, &pool::pkfee>>
         > poolstable;

         typedef eosio::multi_index< "holdertknreq"_n, holderTknReq, 
            eosio::indexed_by< "tid"_n, eosio::const_mem_fun<holderTknReq, uint64_t, &holderTknReq::pktid>>
         > holderTknReqs;
         
         typedef eosio::multi_index< "pooltokenreq"_n, poolTokenReq, 
            eosio::indexed_by< "tid"_n, eosio::const_mem_fun<poolTokenReq, uint64_t, &poolTokenReq::pktid>>,
            eosio::indexed_by< "pool"_n, eosio::const_mem_fun<poolTokenReq, uint64_t, &poolTokenReq::pkpool>>
         > poolTokenReqs;


         asset sqroot(asset number);
         void sub_balance( const name& owner, const asset& value );
         void add_balance( const name& owner, const asset& value, const name& ram_payer );
         void sub_balance2( const name& owner, const asset& value, const name& ram_payer );
         void add_balance2( const name& owner, const asset& value, const name& ram_payer );
         void transfer2Esc( const name& from, const name& to, const asset& quantity, const string& memo );
         void updhldrtkns( const int p_TID, const asset p_tokensRemaining, const name p_hotel, const name p_pool, const double p_reward, const double p_holderPerc, const uint32_t lockedUntil );
         void updpoltottkn( const name& poolName, const uint64_t& poolID, const asset& tokens, const bool& increment );

         void calldeferred( uint32_t delay, uint128_t sender_id );

      //-- END OF PRIVATE REGION

   }; //-- end of class

} //-- end of namespace eosio