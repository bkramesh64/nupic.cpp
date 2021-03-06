/* ---------------------------------------------------------------------
 * Copyright (C) 2019, David McDougall.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 * ----------------------------------------------------------------------
 */

/** @file
 * Definitions for SDR tools & helper classes
 */

#ifndef SDR_TOOLS_HPP
#define SDR_TOOLS_HPP

#include <vector>
#include <nupic/types/Sdr.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/types/Serializable.hpp>

namespace nupic {
namespace sdr {


class ReadOnly_ : public SDR
{
public:
    ReadOnly_() {}

    ReadOnly_( const std::vector<UInt> dimensions )
        : SDR( dimensions ) {}

private:
    const std::string _error_message = "This SDR is read only.";

    void setDenseInplace() const override
        { NTA_THROW << _error_message; }
    void setSparseInplace() const override
        { NTA_THROW << _error_message; }
    void setCoordinatesInplace() const override
        { NTA_THROW << _error_message; }
    void setSDR( const SparseDistributedRepresentation &value ) override
        { NTA_THROW << _error_message; }
    void load(std::istream &inStream) override
        { NTA_THROW << _error_message; }
};


/**
 * Reshape class
 *
 * ### Description
 * Reshape presents a view onto an SDR with different dimensions.
 *      + Reshape is a subclass of SDR and be safely typecast to an SDR.
 *      + The resulting SDR has the same value as the source SDR, at all times
 *        and automatically.
 *      + The resulting SDR is read only.
 *
 * SDR and Reshape classes tell each other when they are created and
 * destroyed.  Reshape can be created and destroyed as needed.  Reshape
 * will throw an exception if it is used after its source SDR has been
 * destroyed.
 *
 * Example Usage:
 *      // Convert SDR dimensions from (4 x 4) to (8 x 2)
 *      SDR     A(    { 4, 4 })
 *      Reshape B( A, { 8, 2 })
 *      A.setCoordinates( {1, 1, 2}, {0, 1, 2}} )
 *      B.getCoordinates()  ->  {{2, 2, 5}, {0, 1, 0}}
 *
 * Reshape partially supports the Serializable interface.  Reshape can
 * be saved but can not be loaded.
 *
 * Note: Reshape used to be called SDR_Proxy. See PR #298
 */
class Reshape : public ReadOnly_
{
public:
    /**
     * Reshape an SDR.
     *
     * @param sdr Source SDR to make a view of.
     *
     * @param dimensions A list of dimension sizes, defining the shape of the
     * SDR.  Optional, if not given then this SDR will have the same
     * dimensions as the given SDR.
     */
    Reshape(SDR &sdr, const std::vector<UInt> &dimensions);

    Reshape(SDR &sdr)
        : Reshape(sdr, sdr.dimensions) {}

    SDR_dense_t& getDense() const override;

    SDR_sparse_t& getSparse() const override;

    SDR_coordinate_t& getCoordinates() const override;

    void save(std::ostream &outStream) const override;

    ~Reshape() override
        { deconstruct(); }

protected:
    /**
     * This SDR shall always have the same value as the parent SDR.
     */
    SDR *parent;
    UInt callback_handle;
    UInt destroyCallback_handle;

    void deconstruct() override;
};


/**
 * Concatenation class
 *
 * ### Description
 * This class presents a view onto a group of SDRs, which always shows the
 * concatenation of them.  This view is read-only.
 *
 * Parameter UInt axis: This can concatenate along any axis, with the
 *      restriction that the result must be rectangular.  The default axis is 0.
 *
 * An Concatenation is valid for as long as all of its input SDRs are alive.
 * Using it after any of it's inputs are destroyed is undefined.
 *
 * Example Usage:
 *      SDR           A({ 100 });
 *      SDR           B({ 100 });
 *      Concatenation C( A, B );
 *      C.dimensions -> { 200 }
 *
 *      SDR           D({ 640, 480, 3 });
 *      SDR           E({ 640, 480, 7 });
 *      Concatenation F( D, E, 2 );
 *      F.dimensions -> { 640, 480, 10 }
 */
class Concatenation : public ReadOnly_
{
public:
    Concatenation(SDR &inp1, SDR &inp2, UInt axis=0u)
        { initialize({&inp1,     &inp2},     axis); }

    Concatenation(SDR &inp1, SDR &inp2, SDR &inp3, UInt axis=0u)
        { initialize({&inp1,     &inp2,     &inp3},     axis); }

    Concatenation(SDR &inp1, SDR &inp2, SDR &inp3, SDR &inp4, UInt axis=0u)
        { initialize({&inp1,     &inp2,     &inp3,     &inp4},     axis); }

    Concatenation(std::vector<SDR*> inputs, UInt axis=0u)
        { initialize(inputs, axis); }

    void initialize(const std::vector<SDR*> inputs, const UInt axis=0u);

    const UInt              &axis   = axis_;
    const std::vector<SDR*> &inputs = inputs_;

    SDR_dense_t& getDense() const override;

    ~Concatenation()
        { deconstruct(); }

protected:
    UInt              axis_;
    std::vector<SDR*> inputs_;
    std::vector<UInt> callback_handles_;
    std::vector<UInt> destroyCallback_handles_;
    mutable bool dense_valid_lazy;

    void clear() const override;

    void deconstruct() override;
};


/**
 * Intersection class
 *
 * This class presents a view onto a group of SDRs, which always shows the set
 * intersection of the active bits in each input SDR.  This view is read-only.
 *
 * Example Usage:
 *     // Setup 2 SDRs to hold the inputs.
 *     SDR A({ 10 });
 *     SDR B({ 10 });
 *     A.setSparse(      {2, 3, 4, 5});
 *     B.setSparse({0, 1, 2, 3});
 *
 *     // Calculate the logical intersection
 *     Intersection X(A, B);
 *     X.getSparse() -> {2, 3}
 *
 *     // Assignments to the input SDRs are propigated to the Intersection
 *     B.zero();
 *     X.getSparsity() -> 0.0
 */
class Intersection : public ReadOnly_
{
public:
    Intersection(SDR &input1, SDR &input2)
        { initialize({   &input1,     &input2}); }
    Intersection(SDR &input1, SDR &input2, SDR &input3)
        { initialize({   &input1,     &input2,     &input3}); }
    Intersection(SDR &input1, SDR &input2, SDR &input3, SDR &input4)
        { initialize({   &input1,     &input2,     &input3,     &input4}); }

    Intersection(std::vector<SDR*> inputs)
        { initialize(inputs); }

    void initialize(const std::vector<SDR*> inputs);

    const std::vector<SDR*> &inputs = inputs_;

    SDR_dense_t& getDense() const override;

    ~Intersection()
        { deconstruct(); }

protected:
    std::vector<SDR*> inputs_;
    std::vector<UInt> callback_handles_;
    std::vector<UInt> destroyCallback_handles_;
    mutable bool      dense_valid_lazy;

    void clear() const override;

    void deconstruct() override;
};


} // end namespace sdr
} // end namespace nupic
#endif // end ifndef SDR_TOOLS_HPP
