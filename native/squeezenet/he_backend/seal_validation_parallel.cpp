#include "squeezenet/he_backend/parallel_context.hpp"
namespace {
template<class M> bool rns_valid110(const char* label,const uint64_t* ptr,size_t n,const M& mods,size_t polys){
 std::atomic<bool> valid{true};const size_t q=mods.size();
 pcn_span091::each(label,n*q*polys,[&](size_t i){if(ptr[i]>=mods[(i/n)%q].value())valid.store(false,std::memory_order_relaxed);});
 return valid.load(std::memory_order_relaxed);
}
}
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "seal/ciphertext.h"
#include "seal/galoiskeys.h"
#include "seal/kswitchkeys.h"
#include "seal/plaintext.h"
#include "seal/publickey.h"
#include "seal/relinkeys.h"
#include "seal/secretkey.h"
#include "seal/valcheck.h"
#include "seal/util/common.h"
#include "seal/util/defines.h"

using namespace std;
using namespace seal::util;

namespace seal
{
    bool is_metadata_valid_for(const Plaintext &in, const SEALContext &context, bool allow_pure_key_levels)
    {
        // Verify parameters
        if (!context.parameters_set())
        {
            return false;
        }

        if (in.is_ntt_form())
        {
            // Are the parameters valid for the plaintext?
            auto context_data_ptr = context.get_context_data(in.parms_id());
            if (!context_data_ptr)
            {
                return false;
            }

            // Check whether the parms_id is in the pure key range
            bool is_parms_pure_key = context_data_ptr->chain_index() > context.first_context_data()->chain_index();
            if (!allow_pure_key_levels && is_parms_pure_key)
            {
                return false;
            }

            auto &parms = context_data_ptr->parms();
            auto &coeff_modulus = parms.coeff_modulus();
            size_t poly_modulus_degree = parms.poly_modulus_degree();

            // Check that coeff_count is appropriately set
            if (mul_safe(coeff_modulus.size(), poly_modulus_degree) != in.coeff_count())
            {
                return false;
            }
        }
        else
        {
            auto &parms = context.first_context_data()->parms();
            size_t poly_modulus_degree = parms.poly_modulus_degree();
            if (in.coeff_count() > poly_modulus_degree)
            {
                return false;
            }
        }

        return true;
    }

    bool is_metadata_valid_for(const Ciphertext &in, const SEALContext &context, bool allow_pure_key_levels)
    {
        // Verify parameters
        if (!context.parameters_set())
        {
            return false;
        }

        // Are the parameters valid for the ciphertext?
        auto context_data_ptr = context.get_context_data(in.parms_id());
        if (!context_data_ptr)
        {
            return false;
        }

        // Check whether the parms_id is in the pure key range
        bool is_parms_pure_key = context_data_ptr->chain_index() > context.first_context_data()->chain_index();
        if (!allow_pure_key_levels && is_parms_pure_key)
        {
            return false;
        }

        // Check that the metadata matches
        auto &coeff_modulus = context_data_ptr->parms().coeff_modulus();
        size_t poly_modulus_degree = context_data_ptr->parms().poly_modulus_degree();
        if ((coeff_modulus.size() != in.coeff_modulus_size()) || (poly_modulus_degree != in.poly_modulus_degree()))
        {
            return false;
        }

        // Check that size is either 0 or within right bounds
        auto size = in.size();
        if ((size < SEAL_CIPHERTEXT_SIZE_MIN && size != 0) || size > SEAL_CIPHERTEXT_SIZE_MAX)
        {
            return false;
        }

        // Check that scale is 1.0 in BFV and BGV or not 0.0 in CKKS
        double scale = in.scale();
        scheme_type scheme = context.first_context_data()->parms().scheme();
        if ((scale != 1.0 && (scheme == scheme_type::bfv || scheme == scheme_type::bgv)) ||
            (scale == 0.0 && scheme == scheme_type::ckks))
        {
            return false;
        }

        // Check that correction factor is 1 in BFV and CKKS or within the right bound in BGV
        uint64_t correction_factor = in.correction_factor();
        uint64_t plain_modulus = context.first_context_data()->parms().plain_modulus().value();
        if ((correction_factor != 1 && (scheme == scheme_type::bfv || scheme == scheme_type::ckks)) ||
            ((correction_factor == 0 || correction_factor >= plain_modulus) && scheme == scheme_type::bgv))
        {
            return false;
        }

        return true;
    }

    bool is_metadata_valid_for(const SecretKey &in, const SEALContext &context)
    {
        // Note: we check the underlying Plaintext and allow pure key levels in
        // this check. Then, also need to check that the parms_id matches the
        // key level parms_id; this also means the Plaintext is in NTT form.
        auto key_parms_id = context.key_parms_id();
        return is_metadata_valid_for(in.data(), context, true) && (in.parms_id() == key_parms_id);
    }

    bool is_metadata_valid_for(const PublicKey &in, const SEALContext &context)
    {
        // Note: we check the underlying Ciphertext and allow pure key levels in
        // this check. Then, also need to check that the parms_id matches the
        // key level parms_id, that the Ciphertext is in NTT form, and that the
        // size is minimal (i.e., SEAL_CIPHERTEXT_SIZE_MIN).
        auto key_parms_id = context.key_parms_id();
        return is_metadata_valid_for(in.data(), context, true) && in.data().is_ntt_form() &&
               (in.parms_id() == key_parms_id) && (in.data().size() == SEAL_CIPHERTEXT_SIZE_MIN);
    }

    bool is_metadata_valid_for(const KSwitchKeys &in, const SEALContext &context)
    {
        // Verify parameters
        if (!context.parameters_set())
        {
            return false;
        }

        // Are the parameters valid and at key level?
        if (in.parms_id() != context.key_parms_id())
        {
            return false;
        }

        size_t decomp_mod_count = context.first_context_data()->parms().coeff_modulus().size();
        for (auto &a : in.data())
        {
            // Check that each highest level component has right size
            if (a.size() && (a.size() != decomp_mod_count))
            {
                return false;
            }
            for (auto &b : a)
            {
                // Check that b is a valid public key (metadata only); this also
                // checks that its parms_id matches key_parms_id.
                if (!is_metadata_valid_for(b, context))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_metadata_valid_for(const RelinKeys &in, const SEALContext &context)
    {
        // Check that the size is within bounds.
        bool size_check =
            !in.size() || (in.size() <= SEAL_CIPHERTEXT_SIZE_MAX - 2 && in.size() >= SEAL_CIPHERTEXT_SIZE_MIN - 2);
        return is_metadata_valid_for(static_cast<const KSwitchKeys &>(in), context) && size_check;
    }

    bool is_metadata_valid_for(const GaloisKeys &in, const SEALContext &context)
    {
        // Check the metadata; then we know context is OK
        bool metadata_check = is_metadata_valid_for(static_cast<const KSwitchKeys &>(in), context);
        bool size_check = !in.size() || in.size() <= context.key_context_data()->parms().poly_modulus_degree();
        return metadata_check && size_check;
    }

    bool is_buffer_valid(const Plaintext &in)
    {
        if (in.coeff_count() != in.dyn_array().size())
        {
            return false;
        }

        return true;
    }

    bool is_buffer_valid(const Ciphertext &in)
    {
        // Check that the buffer size is correct
        if (in.dyn_array().size() != mul_safe(in.size(), in.coeff_modulus_size(), in.poly_modulus_degree()))
        {
            return false;
        }

        return true;
    }

    bool is_buffer_valid(const SecretKey &in)
    {
        return is_buffer_valid(in.data());
    }

    bool is_buffer_valid(const PublicKey &in)
    {
        return is_buffer_valid(in.data());
    }

    bool is_buffer_valid(const KSwitchKeys &in)
    {
        for (auto &a : in.data())
        {
            for (auto &b : a)
            {
                if (!is_buffer_valid(b))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_buffer_valid(const RelinKeys &in)
    {
        return is_buffer_valid(static_cast<const KSwitchKeys &>(in));
    }

    bool is_buffer_valid(const GaloisKeys &in)
    {
        return is_buffer_valid(static_cast<const KSwitchKeys &>(in));
    }

    bool is_data_valid_for(const Plaintext &in, const SEALContext &context)
    {
        // Check metadata
        if (!is_metadata_valid_for(in, context))
        {
            return false;
        }

        if(in.is_ntt_form()){
            const auto& p=context.get_context_data(in.parms_id())->parms();
            return rns_valid110("SEAL_validate_plain_RNS",in.data(),p.poly_modulus_degree(),p.coeff_modulus(),1);
        }
        const auto modulus=context.first_context_data()->parms().plain_modulus().value();std::atomic<bool> valid{true};
        pcn_span091::each("SEAL_validate_plain",in.coeff_count(),[&](size_t i){if(in[i]>=modulus)valid.store(false,std::memory_order_relaxed);});
        return valid.load(std::memory_order_relaxed);
    }

    bool is_data_valid_for(const Ciphertext &in, const SEALContext &context)
    {
        // Check metadata
        if (!is_metadata_valid_for(in, context))
        {
            return false;
        }

        const auto& p=context.get_context_data(in.parms_id())->parms();
        return rns_valid110("SEAL_validate_Ciphertext",in.data(),in.poly_modulus_degree(),p.coeff_modulus(),in.size());
    }

    bool is_data_valid_for(const SecretKey &in, const SEALContext &context)
    {
        // Check metadata
        if (!is_metadata_valid_for(in, context))
        {
            return false;
        }

        const auto& p=context.key_context_data()->parms();
        return rns_valid110("SEAL_validate_SecretKey",in.data().data(),p.poly_modulus_degree(),p.coeff_modulus(),1);
    }

    bool is_data_valid_for(const PublicKey &in, const SEALContext &context)
    {
        // Check metadata
        if (!is_metadata_valid_for(in, context))
        {
            return false;
        }

        const auto& p=context.key_context_data()->parms();
        return rns_valid110("SEAL_validate_PublicKey",in.data().data(),in.data().poly_modulus_degree(),p.coeff_modulus(),in.data().size());
    }

    bool is_data_valid_for(const KSwitchKeys &in, const SEALContext &context)
    {
        // Verify parameters
        if (!context.parameters_set())
        {
            return false;
        }

        // Are the parameters valid for given relinearization keys?
        if (in.parms_id() != context.key_parms_id())
        {
            return false;
        }

        for (auto &a : in.data())
        {
            for (auto &b : a)
            {
                // Check that b is a valid public key; this also checks that its
                // parms_id matches key_parms_id.
                if (!is_data_valid_for(b, context))
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool is_data_valid_for(const RelinKeys &in, const SEALContext &context)
    {
        return is_data_valid_for(static_cast<const KSwitchKeys &>(in), context);
    }

    bool is_data_valid_for(const GaloisKeys &in, const SEALContext &context)
    {
        return is_data_valid_for(static_cast<const KSwitchKeys &>(in), context);
    }
} // namespace seal
