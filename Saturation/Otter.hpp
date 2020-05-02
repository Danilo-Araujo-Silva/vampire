
/*
 * File Otter.hpp.
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
 * @file Otter.hpp
 * Defines class Otter.
 */


#ifndef __Otter__
#define __Otter__

#include "Forwards.hpp"

#include "SaturationAlgorithm.hpp"

namespace Saturation {

using namespace Kernel;

class Otter
: public SaturationAlgorithm
{
public:
  CLASS_NAME(Otter);
  USE_ALLOCATOR(Otter);

  Otter(Problem& prb, const Options& opt);

  virtual ClauseContainer* getSimplifyingClauseContainer() override { return &_simplCont; };
  virtual ClauseContainer* getToBeSimplifiedClauseContainer() override { return &_simplCont; };

protected:

  virtual void onSOSClauseAdded(Clause* cl) override;
  virtual void onActiveRemoved(Clause* cl) override;
  virtual void onPassiveAdded(Clause* cl) override;
  virtual void onPassiveRemoved(Clause* cl) override;
  virtual void onClauseRetained(Clause* cl) override;
  virtual void handleUnsuccessfulActivation(Clause* c) override;

  /**
   * Dummy container for simplification indexes to subscribe
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

  FakeContainer _simplCont;
};

};

#endif /* __Otter__ */
