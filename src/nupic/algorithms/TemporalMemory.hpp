/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2016, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
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
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/** @file
 * Definitions for the Temporal Memory in C++
 */

#ifndef NTA_TEMPORAL_MEMORY_HPP
#define NTA_TEMPORAL_MEMORY_HPP

#include <nupic/algorithms/Connections.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/types/Sdr.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/utils/Random.hpp>
#include <vector>


namespace nupic {
namespace algorithms {
namespace temporal_memory {


using namespace std;
using namespace nupic;
using nupic::algorithms::connections::Connections;
using nupic::algorithms::connections::Permanence;
using nupic::algorithms::connections::Segment;
using nupic::algorithms::connections::CellIdx;
using nupic::algorithms::connections::Synapse;

/**
 * Temporal Memory implementation in C++.
 *
 * Example usage:
 *
 *     SpatialPooler sp(inputDimensions, columnDimensions, <parameters>);
 *     TemporalMemory tm(columnDimensions, <parameters>);
 *
 *     while (true) {
 *        <get input vector, streaming spatiotemporal information>
 *        sp.compute(inputVector, learn, activeColumns)
 *        tm.compute(number of activeColumns, activeColumns, learn)
 *        <do something with the tm, e.g. classify tm.getActiveCells()>
 *     }
 *
 * The public API uses C arrays, not std::vectors, as inputs. C arrays are
 * a good lowest common denominator. You can get a C array from a vector,
 * but you can't get a vector from a C array without copying it. This is
 * important, for example, when using numpy arrays. The only way to
 * convert a numpy array into a std::vector is to copy it, but you can
 * access a numpy array's internal C array directly.
 */
    class TemporalMemory : public Serializable
{
public:
  TemporalMemory();

  /**
   * Initialize the temporal memory (TM) using the given parameters.
   *
   * @param columnDimensions
   * Dimensions of the column space
   *
   * @param cellsPerColumn
   * Number of cells per column
   *
   * @param activationThreshold
   * If the number of active connected synapses on a segment is at least
   * this threshold, the segment is said to be active.
   *
   * @param initialPermanence
   * Initial permanence of a new synapse.
   *
   * @param connectedPermanence
   * If the permanence value for a synapse is greater than this value, it
   * is said to be connected.
   *
   * @param minThreshold
   * If the number of potential synapses active on a segment is at least
   * this threshold, it is said to be "matching" and is eligible for
   * learning.
   *
   * @param maxNewSynapseCount
   * The maximum number of synapses added to a segment during learning.
   *
   * @param permanenceIncrement
   * Amount by which permanences of synapses are incremented during
   * learning.
   *
   * @param permanenceDecrement
   * Amount by which permanences of synapses are decremented during
   * learning.
   *
   * @param predictedSegmentDecrement
   * Amount by which segments are punished for incorrect predictions.
   *
   * @param seed
   * Seed for the random number generator.
   *
   * @param maxSegmentsPerCell
   * The maximum number of segments per cell.
   *
   * @param maxSynapsesPerSegment
   * The maximum number of synapses per segment.
   *
   * @param checkInputs
   * Whether to check that the activeColumns are sorted without
   * duplicates. Disable this for a small speed boost.
   *
   * @param extra
   * Number of external predictive inputs.  These inputs are used in addition to
   * the cells which are part of this TemporalMemory.  The TemporalMemory
   * requires all external inputs be identified by an index in the
   * range [0, extra).
   *
   * Notes:
   *
   * predictedSegmentDecrement: A good value is just a bit larger than
   * (the column-level sparsity * permanenceIncrement). So, if column-level
   * sparsity is 2% and permanenceIncrement is 0.01, this parameter should be
   * something like 4% * 0.01 = 0.0004).
   */
  TemporalMemory(
      vector<UInt>    columnDimensions,
      UInt            cellsPerColumn              = 32,
      UInt            activationThreshold         = 13,
      Permanence      initialPermanence           = 0.21,
      Permanence      connectedPermanence         = 0.50,
      UInt            minThreshold                = 10,
      UInt            maxNewSynapseCount          = 20,
      Permanence      permanenceIncrement         = 0.10,
      Permanence      permanenceDecrement         = 0.10,
      Permanence      predictedSegmentDecrement   = 0.0,
      Int             seed                        = 42,
      UInt            maxSegmentsPerCell          = 255,
      UInt            maxSynapsesPerSegment       = 255,
      bool            checkInputs                 = true,
      UInt            extra                       = 0);

  virtual void
  initialize(
    vector<UInt>  columnDimensions            = {2048},
    UInt          cellsPerColumn              = 32,
    UInt          activationThreshold         = 13,
    Permanence    initialPermanence           = 0.21,
    Permanence    connectedPermanence         = 0.50,
    UInt          minThreshold                = 10,
    UInt          maxNewSynapseCount          = 20,
    Permanence    permanenceIncrement         = 0.10,
    Permanence    permanenceDecrement         = 0.10,
    Permanence    predictedSegmentDecrement   = 0.0,
    Int           seed                        = 42,
    UInt          maxSegmentsPerCell          = 255,
    UInt          maxSynapsesPerSegment       = 255,
    bool          checkInputs                 = true,
    UInt          extra                       = 0);

  virtual ~TemporalMemory();

  //----------------------------------------------------------------------
  //  Main functions
  //----------------------------------------------------------------------

  /**
   * Get the version number of for the TM implementation.
   *
   * @returns Integer version number.
   */
  virtual UInt version() const;

  /**
   * This *only* updates _rng to a new Random using seed.
   */
  void seed_(UInt64 seed);

  /**
   * Indicates the start of a new sequence.
   * Resets sequence state of the TM.
   */
  virtual void reset();

  /**
   * Calculate the active cells, using the current active columns and
   * dendrite segments. Grow and reinforce synapses.
   *
   * @param activeColumnsSize
   * Size of activeColumns array (the 2nd param)
   *
   * @param activeColumns
   * A sorted list of active column indices.
   *
   * @param learn
   * If true, reinforce / punish / grow synapses.
   */
  void activateCells(const size_t activeColumnsSize, const UInt activeColumns[],
                     bool learn = true);
  void activateCells(const sdr::SDR &activeColumns, bool learn = true);

  /**
   * Calculate dendrite segment activity, using the current active cells.  Call
   * this method before calling getPredictiveCells, getActiveSegments, or
   * getMatchingSegments.  In each time step, only the first call to this
   * method has an effect, subsequent calls assume that the prior results are
   * still valid.
   *
   * @param learn
   * If true, segment activations will be recorded. This information is
   * used during segment cleanup.
   *
   * @param extraActive
   * Vector of active external predictive inputs.  External inputs must be cell
   * indexes in the range [0, extra).
   *
   * @param extraWinners
   * Vector of winning external predictive inputs.  When learning, only these
   * inputs are considered active.  ExtraWinners should be a subset of
   * extraActive.  External inputs must be cell indexes in the range [0,
   * extra).
   */
  void activateDendrites(bool learn = true,
                         const vector<UInt> &extraActive  = {std::numeric_limits<UInt>::max()},
                         const vector<UInt> &extraWinners = {std::numeric_limits<UInt>::max()});
  void activateDendrites(bool learn,
                         const sdr::SDR &extraActive, const sdr::SDR &extraWinners);

  /**
   * Perform one time step of the Temporal Memory algorithm.
   *
   * This method calls activateDendrites, then calls activateCells. Using
   * the TemporalMemory via its compute method ensures that you'll always
   * be able to call getActiveCells at the end of the time step.
   *
   * @param activeColumnsSize
   * Number of active columns.
   *
   * @param activeColumns
   * Sorted list of indices of active columns.
   *
   * @param learn
   * Whether or not learning is enabled.
   *
   * @param extraActive
   * Vector of active external predictive inputs.  External inputs must be cell
   * indexes in the range [0, extra).
   *
   * @param extraWinners
   * Vector of winning external predictive inputs.  When learning, only these
   * inputs are considered active.  ExtraWinners should be a subset of
   * extraActive.  External inputs must be cell indexes in the range [0,
   * extra).
   */
  virtual void compute(size_t activeColumnsSize, const UInt activeColumns[],
                       bool learn = true,
                       const vector<UInt> &extraActive  = {std::numeric_limits<UInt>::max()},
                       const vector<UInt> &extraWinners = {std::numeric_limits<UInt>::max()});
  virtual void compute(const sdr::SDR &activeColumns, bool learn,
                       const sdr::SDR &extraActive, const sdr::SDR &extraWinners);

  // ==============================
  //  Helper functions
  // ==============================

  /**
   * Create a segment on the specified cell. This method calls
   * createSegment on the underlying connections, and it does some extra
   * bookkeeping. Unit tests should call this method, and not
   * connections.createSegment().
   *
   * @param cell
   * Cell to add a segment to.
   *
   * @return Segment
   * The created segment.
   */
  Segment createSegment(CellIdx cell);

  /**
   * Returns the indices of cells that belong to a column.
   *
   * @param column Column index
   *
   * @return (vector<CellIdx>) Cell indices
   */
  vector<CellIdx> cellsForColumn(Int column);

  /**
   * Returns the number of cells in this layer.
   *
   * @return (int) Number of cells
   */
  UInt numberOfCells(void) const;

  /**
   * Returns the indices of the active cells.
   *
   * @returns (std::vector<CellIdx>) Vector of indices of active cells.
   */
  vector<CellIdx> getActiveCells() const;
  void getActiveCells(sdr::SDR &activeCells) const;

  /**
   * Returns the indices of the predictive cells.
   *
   * @returns (std::vector<CellIdx>) Vector of indices of predictive cells.
   */
  vector<CellIdx> getPredictiveCells() const;
  void getPredictiveCells(sdr::SDR &predictiveCells) const;

  /**
   * Returns the indices of the winner cells.
   *
   * @returns (std::vector<CellIdx>) Vector of indices of winner cells.
   */
  vector<CellIdx> getWinnerCells() const;
  void getWinnerCells(sdr::SDR &winnerCells) const;

  vector<Segment> getActiveSegments() const;
  vector<Segment> getMatchingSegments() const;

  /**
   * Returns the dimensions of the columns in the region.
   *
   * @returns Integer number of column dimension
   */
  vector<UInt> getColumnDimensions() const;

  /**
   * Returns the total number of columns.
   *
   * @returns Integer number of column numbers
   */
  UInt numberOfColumns() const;

  /**
   * Returns the number of cells per column.
   *
   * @returns Integer number of cells per column
   */
  UInt getCellsPerColumn() const;

  /**
   * Returns the activation threshold.
   *
   * @returns Integer number of the activation threshold
   */
  UInt getActivationThreshold() const;
  void setActivationThreshold(UInt);

  /**
   * Returns the initial permanence.
   *
   * @returns Initial permanence
   */
  Permanence getInitialPermanence() const;
  void setInitialPermanence(Permanence);

  /**
   * Returns the connected permanance.
   *
   * @returns Returns the connected permanance
   */
  Permanence getConnectedPermanence() const;

  /**
   * Returns the minimum threshold.
   *
   * @returns Integer number of minimum threshold
   */
  UInt getMinThreshold() const;
  void setMinThreshold(UInt);

  /**
   * Returns the maximum number of synapses that can be added to a segment
   * in a single time step.
   *
   * @returns Integer number of maximum new synapse count
   */
  UInt getMaxNewSynapseCount() const;
  void setMaxNewSynapseCount(UInt);

  /**
   * Get and set the checkInputs parameter.
   */
  bool getCheckInputs() const;
  void setCheckInputs(bool);

  /**
   * Returns the permanence increment.
   *
   * @returns Returns the Permanence increment
   */
  Permanence getPermanenceIncrement() const;
  void setPermanenceIncrement(Permanence);

  /**
   * Returns the permanence decrement.
   *
   * @returns Returns the Permanence decrement
   */
  Permanence getPermanenceDecrement() const;
  void setPermanenceDecrement(Permanence);

  /**
   * Returns the predicted Segment decrement.
   *
   * @returns Returns the segment decrement
   */
  Permanence getPredictedSegmentDecrement() const;
  void setPredictedSegmentDecrement(Permanence);

  /**
   * Returns the maxSegmentsPerCell.
   *
   * @returns Max segments per cell
   */
  UInt getMaxSegmentsPerCell() const;

  /**
   * Returns the maxSynapsesPerSegment.
   *
   * @returns Max synapses per segment
   */
  UInt getMaxSynapsesPerSegment() const;

  /**
   * Save (serialize) the current state of the spatial pooler to the
   * specified file.
   *
   * @param fd A valid file descriptor.
   */
  virtual void save(ostream &outStream) const override;


  /**
   * Load (deserialize) and initialize the spatial pooler from the
   * specified input stream.
   *
   * @param inStream A valid istream.
   */
  virtual void load(istream &inStream) override;

  bool operator==(const TemporalMemory &other);
  bool operator!=(const TemporalMemory &other);

  //----------------------------------------------------------------------
  // Debugging helpers
  //----------------------------------------------------------------------

  /**
   * Print the main TM creation parameters
   */
  void printParameters();

  /**
   * Returns the index of the column that a cell belongs to.
   *
   * @param cell Cell index
   *
   * @return (int) Column index
   */
  UInt columnForCell(const CellIdx cell) const;

protected:
  UInt numColumns_;
  vector<UInt> columnDimensions_;
  UInt cellsPerColumn_;
  UInt activationThreshold_;
  UInt minThreshold_;
  UInt maxNewSynapseCount_;
  bool checkInputs_;
  Permanence initialPermanence_;
  Permanence connectedPermanence_;
  Permanence permanenceIncrement_;
  Permanence permanenceDecrement_;
  Permanence predictedSegmentDecrement_;
  UInt extra_;

  vector<CellIdx> activeCells_;
  vector<CellIdx> winnerCells_;
  bool segmentsValid_;
  vector<Segment> activeSegments_;
  vector<Segment> matchingSegments_;
  vector<UInt32> numActiveConnectedSynapsesForSegment_;
  vector<UInt32> numActivePotentialSynapsesForSegment_;

  UInt maxSegmentsPerCell_;
  UInt maxSynapsesPerSegment_;
  UInt64 iteration_;
  vector<UInt64> lastUsedIterationForSegment_;

  Random rng_;

public:
  Connections connections;
};

} // end namespace temporal_memory
} // end namespace algorithms
} // namespace nupic

#endif // NTA_TEMPORAL_MEMORY_HPP
