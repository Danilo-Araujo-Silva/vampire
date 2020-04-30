
/*
 * File Discount.cpp.
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
 * @file Discount.cpp
 * Implements class Discount.
 */

#include "Lib/Environment.hpp"
#include "Lib/VirtualIterator.hpp"
#include "Kernel/Clause.hpp"
#include "Kernel/LiteralSelector.hpp"
#include "Shell/Options.hpp"
#include "Shell/Statistics.hpp"

#include "Discount.hpp"

using namespace Lib;
using namespace Kernel;
using namespace Shell;
using namespace Saturation;


bool Discount::handleClauseBeforeActivation(Clause* cl)
{
  CALL("Discount::handleClauseBeforeActivation");
  ASS(cl->store()==Clause::SELECTED);

  if (!forwardSimplify(cl)) {
    cl->setStore(Clause::NONE);
    return false;
  }
  backwardSimplify(cl);
  return true;
}

bool Discott::handleClauseBeforeActivation(Clause* cl)
{
  CALL("Discott::handleClauseBeforeActivation");
  ASS(cl->store()==Clause::SELECTED);

  // cl transitions from PASSIVE to SELECTED, so remove it from _toBeSimplifiedContainer
  // TODO: it would be conceptually cleaner to remove cl from _toBeSimplifiedContainer already as soon as cl->store() is set to SELECTED, so that we more tightly keep the invariant that _toBeSimplifiedContainer always contains exactly the clauses occuring in either Passive or Active (that is, the clauses where cl-store() is set to PASSIVE resp. ACTIVE).
  _toBeSimplifiedContainer.remove(cl);

  // NOTE: The correctness of the saturation depends on the invariant that whenever forwardSimplify(cl) or backwardSimplify(cl) is called, then cl is not part of the clause container
  // with which the simpifications are performed. This is a necessary invariant, as it excludes the otherwise possible corner case, where a clause simplifies itself, which in general leads
  // to incompleteness.
  backwardSimplify(cl);

  // cl transitions from SELECTED to ACTIVE, so add it to _toBeSimplifiedContainer
  // NOTE: this is quite hacky (a nonhacky solution would need to fix the hacky callback-API currently provided by SaturationAlgorithm):
  //       It is possible that cl doesn't get into Active (if it is deleted during activation, e.g. because it is split, which can happen if Avatar's
  //       splitAtActivation option is on).
  //       This works, as the case where cl is deleted during activation is handled by overriding handleUnsuccessfulActivation and immediatley deleting there the clause we added here.
  //       It is still not optimal, since 1) it could easily get broken by future changes, and 2) it potentially adds a clause to the container and immediately afterwards deletes it.
  _toBeSimplifiedContainer.add(cl);

  return true;
}

void Discott::onActiveRemoved(Clause* cl)
{
  CALL("Discott::onActiveRemoved");

  if(cl->store()==Clause::ACTIVE) {
    _toBeSimplifiedContainer.remove(cl);
  }

  SaturationAlgorithm::onActiveRemoved(cl);
}

void Discott::onPassiveAdded(Clause* cl)
{
  CALL("Discott::onPassiveAdded");

  SaturationAlgorithm::onPassiveAdded(cl);

  if(cl->store()==Clause::PASSIVE) {
    _toBeSimplifiedContainer.add(cl);
  }
}

void Discott::onPassiveRemoved(Clause* cl)
{
  CALL("Discott::onPassiveRemoved");

  if(cl->store()==Clause::PASSIVE) {
    _toBeSimplifiedContainer.remove(cl);
  }

  SaturationAlgorithm::onPassiveRemoved(cl);
}

void Discott::onSOSClauseAdded(Clause* cl)
{
  CALL("Discott::onSOSClauseAdded");
  ASS(cl);
  ASS_EQ(cl->store(), Clause::ACTIVE);

  SaturationAlgorithm::onSOSClauseAdded(cl);

  _toBeSimplifiedContainer.add(cl);
}

void Discott::handleUnsuccessfulActivation(Clause* c)
{
  CALL("Discott::handleUnsuccessfulActivation");

  ASS_EQ(c->store(), Clause::SELECTED);
  _toBeSimplifiedContainer.remove(c);
  c->setStore(Clause::NONE);
}