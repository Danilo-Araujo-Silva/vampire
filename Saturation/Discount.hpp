
/*
 * File Discount.hpp.
 *
 * This file is part of the source code of the software program
 * Vampire. It is protected by applicable
 * copyright laws.
 *
 * This source code is distributed under the licence found here
 * https://vprover.github.io/license.html
 * and in the source directory
 *
 * In summary, you are allowed to use Vampire for non-commercial
 * purposes but not allowed to distribute, modify, copy, create derivatives,
 * or use in competitions. 
 * For other uses of Vampire please contact developers for a different
 * licence, which we will make an effort to provide. 
 */
/**
 * @file Discount.hpp
 * Defines class Discount.
 */


#ifndef __Discount__
#define __Discount__

#include "Forwards.hpp"

#include "SaturationAlgorithm.hpp"

namespace Saturation {

using namespace Kernel;

class Discount
: public SaturationAlgorithm
{
public:
  CLASS_NAME(Discount);
  USE_ALLOCATOR(Discount);

  Discount(Problem& prb, const Options& opt)
    : SaturationAlgorithm(prb, opt) {}

  virtual ClauseContainer* getSimplifyingClauseContainer() override { return _active; };
  virtual ClauseContainer* getToBeSimplifiedClauseContainer() override { return _active; };

protected:

  virtual bool handleClauseBeforeActivation(Clause* cl) override;

};

/*
* Saturation algorithm, which keeps invariant that all clauses in Active&Passive are simplified by clauses in Active
* It maintains this invariant as follows:
* - before a clause is added to passive, it is forward simplified by the clauses in Active (implemented in SaturationAlgorithm::doUnprocessedLoop() )
* - before a clause is added to active, it backward simplifies all clauses in Active and Passive (implemented by overwriting SaturationAlgorithm::handleClauseBeforeActivation() )
*/
class Discott
: public SaturationAlgorithm
{
public:
  CLASS_NAME(Discott);
  USE_ALLOCATOR(Discott);

  Discott(Problem& prb, const Options& opt) : SaturationAlgorithm(prb, opt) {}

  virtual ClauseContainer* getSimplifyingClauseContainer() override { return _active; };
  virtual ClauseContainer* getToBeSimplifiedClauseContainer() override { return &_toBeSimplifiedContainer; };

protected:
  virtual bool handleClauseBeforeActivation(Clause* cl) override;

  virtual void onSOSClauseAdded(Clause* cl) override;
  virtual void onActiveRemoved(Clause* cl) override;
  virtual void onPassiveAdded(Clause* cl) override;
  virtual void onPassiveRemoved(Clause* cl) override;
  virtual void handleUnsuccessfulActivation(Clause* c) override;

  /**
   * Dummy container for backward simplification indices to subscribe
   * to its events.
   */
  struct FakeContainer
  : public ClauseContainer
  {
    /**
     * This method is called by @b saturate() method when a clause
     * makes it from unprocessed to passive container.
     */
    void add(Clause* c)
    { addedEvent.fire(c); }

    /**
     * This method is subscribed to remove events of passive
     * and active container, so it gets called automatically
     * when a clause is removed from one of them. (Clause
     * selection in passive container doesn't count as removal.)
     */
    void remove(Clause* c)
    { removedEvent.fire(c); }
  };

  FakeContainer _toBeSimplifiedContainer;
};

};

#endif /* __Discount__ */
