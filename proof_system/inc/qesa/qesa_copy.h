#pragma once

#include "qesa/internal/qesa_inner.h"
#include <set>

/*
 * QESA copy protocol
 *
 * Proves that Gamma_i * witness == 0 for all i and that m Commitments C_j are computed from value vectors v_j.
 * CRS witness size has to be at least witness.size() + m + v_j.size() for all j.
 * witness[0] has to be 1.
 *
 * In order to create compatible commitments follow these steps:
 *   - Obtain a commitment CRS using get_commitment_crs by specifying the indices of the QESA CRS you want to use.
 *   - Commit to several message vectors using the commitment CRSs you created.
 *   - Compute a QESA copy mapping via compute_mapping, pass the original witness size and the commitment CRSs
 *     in the order they were used. If a CRS was used multiple times, pass it multiple times.
 *
 * Prover input: Matrices Gamma_i, corresponding witness, commitment mapping, and openings (message vector, randomness)
 * Verifier input: Matrices Gamma_i, commitment mapping, and commitments
 */

namespace qesa::copy
{
    using CommitmentCRS = std::vector<u32>;

    struct Mapping;
    struct ProverContext;
    struct VerifierContext;

    // ##################################
    // methods
    // ##################################

    /*
     * Get a new commitment CRS.
     * The given generator_indices are the indices in the vector of generators which are used to commit.
     * Example: indices {0,2} given -> commitment will use g[0] and g[2] to commit to a 2-element message.
     */
    CommitmentCRS get_commitment_crs(const CRS& crs, const std::vector<u32>& generator_indices);

    /*
     * Commitment function that takes a specific commitment crs.
     * All commitments used in this proof have to be generated with this function.
     */
    G commit(const CRS& crs_main, const CommitmentCRS& crs_com, const std::vector<BN>& msg, const BN& r);

    /*
     * Computes mapping data that is necessary to copy the values to their correct positions in the most efficient way.
     * Has to be called only once to generate the mapping required to begin the protocol.
     */
    Mapping compute_mapping(const CRS& crs_main, u32 witness_size, const std::vector<CommitmentCRS>& com_crs_set);


    /*
     * Protocol initiation:
     * Prover and verifier have to initialize their contexts with these functions.
     */
    void begin(ProverContext& ctx, const std::vector<SparseMatrix>& matrices,
               const Mapping& mapping, const math::vector<BN>& witness,
               const std::vector<std::tuple<std::vector<BN>, BN>>& openings);
    void begin(VerifierContext& ctx, const std::vector<SparseMatrix>& matrices,
               const Mapping& mapping, const std::vector<G>& commitments);

    /*
     * Protocol execution:
     * step_prover and step_verifier have to be called alternatingly, step_prover has to be called first.
     * Both functions take their respective context and a buffer for the network packet as input.
     * The buffer is read by the function, cleared, and filled with the new outputs.
     * Both functions return true if the protocol is still running, i.e., if they need to be called again on the partners response.
     */
    bool step_prover(ProverContext& ctx, std::vector<u8>& buffer);
    bool step_verifier(VerifierContext& ctx, std::vector<u8>& buffer);

    /*
     * Given a VerifierContext after protocol execution, this returns true if the proof was correct.
     */
    bool get_result(VerifierContext& ctx);

    // ##################################
    // types
    // ##################################

    struct Mapping
    {
        u32 num_crs;

        std::map<u32, u32> key_position_for_message;
        std::map<u32, u32> msg_id_to_msg_position;
        std::map<u32, u32> crs_id_for_message;

        std::set<u32> unique_key_indices;
        std::map<u32, std::vector<u32>> messages_for_key;

        u32 last_index;
    };

    struct ProverContext : inner::ProverContext
    {
        using inner::ProverContext::ProverContext;

        std::vector<std::tuple<std::vector<BN>, BN>> openings;
        Mapping mapping;

        u32 state;
    };

    struct VerifierContext : inner::VerifierContext
    {
        using inner::VerifierContext::VerifierContext;

        std::vector<G> commitments;
        Mapping mapping;

        u32 state;
    };
}
