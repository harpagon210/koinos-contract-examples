#include <koinos/system/system_calls.hpp>

#include <koinos/buffer.hpp>
#include <koinos/common.h>

#include <boost/multiprecision/cpp_int.hpp>

#include <string>

#include <nft.h>

using namespace koinos;

namespace constants
{

   static const std::string token_name = "Token";
   static const std::string token_symbol = "TOKEN";
   constexpr std::size_t max_address_size = 25;
   constexpr std::size_t max_name_size = 32;
   constexpr std::size_t max_symbol_size = 8;
   constexpr std::size_t max_buffer_size = 2048;
   constexpr uint64_t sha256_id = 0x12;
   constexpr uint64_t ripemd_160_id = 0x1053;
   static const std::array<uint8_t, max_address_size> zero_address = {};

} // constants

namespace state
{
   system::object_space tokens_space()
   {
      system::object_space obj_space;
      auto contract_id = system::get_contract_id();
      obj_space.mutable_zone().set(reinterpret_cast<const uint8_t *>(contract_id.data()), contract_id.size());
      obj_space.set_id(1);
      return obj_space;
   }

   system::object_space balances_space()
   {
      system::object_space obj_space;
      auto contract_id = system::get_contract_id();
      obj_space.mutable_zone().set(reinterpret_cast<const uint8_t *>(contract_id.data()), contract_id.size());
      obj_space.set_id(2);
      return obj_space;
   }

   system::object_space token_approvals_space()
   {
      system::object_space obj_space;
      auto contract_id = system::get_contract_id();
      obj_space.mutable_zone().set(reinterpret_cast<const uint8_t *>(contract_id.data()), contract_id.size());
      obj_space.set_id(3);
      return obj_space;
   }

   system::object_space operator_approvals_space()
   {
      system::object_space obj_space;
      auto contract_id = system::get_contract_id();
      obj_space.mutable_zone().set(reinterpret_cast<const uint8_t *>(contract_id.data()), contract_id.size());
      obj_space.set_id(4);
      return obj_space;
   }
}

enum entries : uint32_t
{
   name_entry = 1,
   symbol_entry = 2,
   balance_of_entry = 3,
   owner_of_entry = 4,
   mint_entry = 5,
   transfer_entry = 6,
   approve_entry = 7,
   set_approval_for_all_entry = 8,
   get_approved_entry = 9,
   is_approved_for_all_entry = 10,
};

// read
using name_arguments = nft::name_arguments;
using name_result = nft::name_result<constants::max_name_size>;
using symbol_arguments = nft::symbol_arguments;
using symbol_result = nft::symbol_result<constants::max_symbol_size>;
using balance_of_arguments = nft::balance_of_arguments<constants::max_address_size>;
using balance_of_result = nft::balance_of_result;
using owner_of_arguments = nft::owner_of_arguments;
using owner_of_result = nft::owner_of_result<constants::max_address_size>;
using get_approved_arguments = nft::get_approved_arguments;
using get_approved_result = nft::get_approved_result<constants::max_address_size>;
using is_approved_for_all_arguments = nft::is_approved_for_all_arguments<constants::max_address_size, constants::max_address_size>;
using is_approved_for_all_result = nft::is_approved_for_all_result;

// write
using mint_arguments = nft::mint_arguments<constants::max_address_size>;
using mint_result = nft::mint_result;
using transfer_arguments = nft::transfer_arguments<constants::max_address_size, constants::max_address_size>;
using transfer_result = nft::transfer_result;
using approve_arguments = nft::approve_arguments<constants::max_address_size, constants::max_address_size>;
using approve_result = nft::approve_result;
using set_approval_for_all_arguments = nft::set_approval_for_all_arguments<constants::max_address_size, constants::max_address_size>;
using set_approval_for_all_result = nft::set_approval_for_all_result;

// objects
using token_object = nft::token_object<constants::max_address_size>;
using balance_object = nft::balance_object;
using token_approval_object = nft::token_approval_object<constants::max_address_size>;
using operator_approval_object = nft::operator_approval_object;

name_result name()
{
   name_result res;

   res.mutable_value() = constants::token_name.c_str();
   return res;
}

symbol_result symbol()
{
   symbol_result res;
   res.mutable_value() = constants::token_symbol.c_str();
   return res;
}

balance_of_result balance_of(const balance_of_arguments &args)
{
   balance_of_result res;

   std::string owner(reinterpret_cast<const char *>(args.get_owner().get_const()), args.get_owner().get_length());

   balance_object bal_obj;
   system::get_object(state::balances_space(), owner, bal_obj);

   res.set_value(bal_obj.get_value());
   return res;
}

owner_of_result owner_of(const owner_of_arguments &args)
{
   owner_of_result res;

   uint64_t token_id = args.get_token_id();

   token_object token_obj;
   system::get_object(state::tokens_space(), std::to_string(token_id), token_obj);

   res.set_value(token_obj.get_owner());
   return res;
}

get_approved_result get_approved(const get_approved_arguments &args)
{
   get_approved_result res;

   uint64_t token_id = args.get_token_id();

   token_approval_object token_approval_obj;
   system::get_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj);

   res.set_value(token_approval_obj.get_address());
   return res;
}

is_approved_for_all_result is_approved_for_all(const is_approved_for_all_arguments &args)
{
   is_approved_for_all_result res;
   res.set_value(false);

   std::string owner(reinterpret_cast<const char *>(args.get_owner().get_const()), args.get_owner().get_length());
   std::string address(reinterpret_cast<const char *>(args.get_address().get_const()), args.get_address().get_length());

   operator_approval_object operator_approval_obj;
   auto approval_key_id = system::hash(constants::ripemd_160_id, owner + address);

   if(system::get_object(state::operator_approvals_space(), approval_key_id, operator_approval_obj)) {
      res.set_value(operator_approval_obj.get_approved());
   }

   return res;
}

mint_result mint(const mint_arguments &args)
{
   mint_result res;
   res.set_value(false);

   std::string to(reinterpret_cast<const char *>(args.get_to().get_const()), args.get_to().get_length());
   uint64_t token_id = args.get_token_id();

   // only this contract can mint new tokens
   system::require_authority(system::get_contract_id());

   // check that the token has not already been minted
   token_object token_obj;
   if (system::get_object(state::tokens_space(), std::to_string(token_id), token_obj))
   {
      system::print("token already minted\n");
      return res;
   }

   // assign the new token's owner
   token_obj.set_owner(args.get_to());

   // update the owner's balance
   balance_object balance_obj;
   system::get_object(state::balances_space(), to, balance_obj);
   balance_obj.set_value(balance_obj.get_value() + 1);

   system::put_object(state::balances_space(), to, balance_obj);
   system::put_object(state::tokens_space(), std::to_string(token_id), token_obj);

   res.set_value(true);
   return res;
}

transfer_result transfer(const transfer_arguments &args)
{
   transfer_result res;
   res.set_value(false);

   std::string from(reinterpret_cast<const char *>(args.get_from().get_const()), args.get_from().get_length());
   std::string to(reinterpret_cast<const char *>(args.get_to().get_const()), args.get_to().get_length());
   uint64_t token_id = args.get_token_id();

   // require authority of the from address
   system::require_authority(from);

   // check that the token exists
   token_object token_obj;
   if (!system::get_object(state::tokens_space(), std::to_string(token_id), token_obj))
   {
      system::print("nonexistent token\n");
      return res;
   }

   std::string owner(reinterpret_cast<const char *>(token_obj.get_owner().get_const()), token_obj.get_owner().get_length());

   if (owner != from)
   {
      token_approval_object token_approval_obj;
      bool is_token_approved = false;

      if (system::get_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj))
      {
         std::string approved_address(reinterpret_cast<const char *>(token_approval_obj.get_address().get_const()), token_approval_obj.get_address().get_length());

         is_token_approved = approved_address == from;
      }

      bool is_operator_approved = false;
      if (!is_token_approved)
      {
         operator_approval_object operator_approval_obj;
         auto approval_key_id = system::hash(constants::ripemd_160_id, owner + from);
         system::get_object(state::operator_approvals_space(), approval_key_id, operator_approval_obj);

         is_operator_approved = operator_approval_obj.get_approved();

         if (!is_operator_approved)
         {
            system::print("transfer caller is not owner nor approved\n");
            return res;
         }
      }
   }

   // clear the token approval
   token_approval_object token_approval_obj;
   system::get_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj);
   token_approval_obj.mutable_address().set(constants::zero_address.data(), constants::max_address_size);
   system::put_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj);

   // update the balances
   // from
   balance_object from_balance_obj;
   system::get_object(state::balances_space(), from, from_balance_obj);
   from_balance_obj.set_value(from_balance_obj.get_value() - 1);
   system::put_object(state::balances_space(), from, from_balance_obj);

   // to
   balance_object to_balance_obj;
   system::get_object(state::balances_space(), to, to_balance_obj);
   to_balance_obj.set_value(to_balance_obj.get_value() + 1);
   system::put_object(state::balances_space(), to, to_balance_obj);

   // assign the new token's owner
   token_obj.set_owner(args.get_to());

   system::put_object(state::tokens_space(), std::to_string(token_id), token_obj);

   res.set_value(true);
   return res;
}

approve_result approve(const approve_arguments &args)
{
   approve_result res;
   res.set_value(false);

   std::string approver_address(reinterpret_cast<const char *>(args.get_approver_address().get_const()), args.get_approver_address().get_length());
   std::string to(reinterpret_cast<const char *>(args.get_to().get_const()), args.get_to().get_length());
   uint64_t token_id = args.get_token_id();

   // require authority of the approver_address
   system::require_authority(approver_address);

   // check that the token exists
   token_object token_obj;
   if (!system::get_object(state::tokens_space(), std::to_string(token_id), token_obj))
   {
      system::print("nonexistent token\n");
      return res;
   }

   std::string owner(reinterpret_cast<const char *>(token_obj.get_owner().get_const()), token_obj.get_owner().get_length());

   // check that the to is not the owner
   if (to == owner)
   {
      system::print("approval to current owner\n");
      return res;
   }

   // check that the approver_address is allowed to approve the token
   if (approver_address != owner)
   {
      operator_approval_object operator_approval_obj;
      auto approval_key_id = system::hash(constants::ripemd_160_id, owner + approver_address);
      system::get_object(state::operator_approvals_space(), approval_key_id, operator_approval_obj);

      if (!operator_approval_obj.get_approved())
      {
         system::print("approver_address is not owner nor approved\n");
         return res;
      }
   }

   token_approval_object token_approval_obj;
   system::get_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj);
   token_approval_obj.set_address(args.get_to());
   system::put_object(state::token_approvals_space(), std::to_string(token_id), token_approval_obj);

   res.set_value(true);
   return res;
}

set_approval_for_all_result set_approval_for_all(const set_approval_for_all_arguments &args)
{
   set_approval_for_all_result res;
   res.set_value(false);

   std::string approver_address(reinterpret_cast<const char *>(args.get_approver_address().get_const()), args.get_approver_address().get_length());
   std::string operator_address(reinterpret_cast<const char *>(args.get_operator_address().get_const()), args.get_operator_address().get_length());
   bool approved = args.get_approved();

   // only the owner of approver_address can approve an operator for his account
   system::require_authority(approver_address);

   // check that the approver_address is not the address to approve
   if (approver_address == operator_address)
   {
      system::print("approve to operator_address\n");
      return res;
   }

   operator_approval_object operator_approval_obj;
   auto approval_key_id = system::hash(constants::ripemd_160_id, approver_address + operator_address);

   system::get_object(state::operator_approvals_space(), approval_key_id, operator_approval_obj);
   operator_approval_obj.set_approved(approved);
   system::put_object(state::operator_approvals_space(), approval_key_id, operator_approval_obj);

   res.set_value(true);
   return res;
}

int main()
{
   auto entry_point = system::get_entry_point();
   auto args = system::get_contract_arguments();

   std::array<uint8_t, constants::max_buffer_size> retbuf;

   koinos::read_buffer rdbuf((uint8_t *)args.c_str(), args.size());
   koinos::write_buffer buffer(retbuf.data(), retbuf.size());

   switch (std::underlying_type_t<entries>(entry_point))
   {
   case entries::name_entry:
   {
      auto res = name();
      res.serialize(buffer);
      break;
   }
   case entries::symbol_entry:
   {
      auto res = symbol();
      res.serialize(buffer);
      break;
   }
   case entries::balance_of_entry:
   {
      balance_of_arguments arg;
      arg.deserialize(rdbuf);

      auto res = balance_of(arg);
      res.serialize(buffer);
      break;
   }
   case entries::owner_of_entry:
   {
      owner_of_arguments arg;
      arg.deserialize(rdbuf);

      auto res = owner_of(arg);
      res.serialize(buffer);
      break;
   }
   case entries::mint_entry:
   {
      mint_arguments arg;
      arg.deserialize(rdbuf);

      auto res = mint(arg);
      res.serialize(buffer);
      break;
   }
   case entries::transfer_entry:
   {
      transfer_arguments arg;
      arg.deserialize(rdbuf);

      auto res = transfer(arg);
      res.serialize(buffer);
      break;
   }
   case entries::approve_entry:
   {
      approve_arguments arg;
      arg.deserialize(rdbuf);

      auto res = approve(arg);
      res.serialize(buffer);
      break;
   }
   case entries::set_approval_for_all_entry:
   {
      set_approval_for_all_arguments arg;
      arg.deserialize(rdbuf);

      auto res = set_approval_for_all(arg);
      res.serialize(buffer);
      break;
   }
   case entries::get_approved_entry:
   {
      get_approved_arguments arg;
      arg.deserialize(rdbuf);

      auto res = get_approved(arg);
      res.serialize(buffer);
      break;
   }
   case entries::is_approved_for_all_entry:
   {
      is_approved_for_all_arguments arg;
      arg.deserialize(rdbuf);

      auto res = is_approved_for_all(arg);
      res.serialize(buffer);
      break;
   }
   default:
      system::exit_contract(1);
   }

   std::string retval(reinterpret_cast<const char *>(buffer.data()), buffer.get_size());
   system::set_contract_result_bytes(retval);

   system::exit_contract(0);
   return 0;
}
