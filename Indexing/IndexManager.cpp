
/*
 * File IndexManager.cpp.
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
 * @file IndexManager.cpp
 * Implements class IndexManager.
 */

#include "Lib/Exception.hpp"

#include "Kernel/Grounder.hpp"

#include "Saturation/SaturationAlgorithm.hpp"

#include "AcyclicityIndex.hpp"
#include "ArithmeticIndex.hpp"
#include "CodeTreeInterfaces.hpp"
#include "GroundingIndex.hpp"
#include "LiteralIndex.hpp"
#include "LiteralSubstitutionTree.hpp"
#include "TermIndex.hpp"
#include "TermSubstitutionTree.hpp"

#include "IndexManager.hpp"

using namespace Lib;
using namespace Indexing;

IndexManager::IndexManager(SaturationAlgorithm* alg) : _alg(alg), _genLitIndex(0), _equalSimpContainers(false)
{
  CALL("IndexManager::IndexManager");

  if(alg) {
    _equalSimpContainers = alg->getSimplifyingClauseContainer() == alg->getToBeSimplifiedClauseContainer();
    attach(alg);
  }
}

IndexManager::~IndexManager()
{
  CALL("IndexManager::~IndexManager");

  if(_alg) {
    release(GENERATING_SUBST_TREE);
  }
}

void IndexManager::setSaturationAlgorithm(SaturationAlgorithm* alg)
{
  CALL("IndexManager::setSaturationAlgorithm");
  ASS(!_alg);
  ASS(alg);

  _alg = alg;
  _equalSimpContainers = alg->getSimplifyingClauseContainer() == alg->getToBeSimplifiedClauseContainer();
  attach(alg);
}

void IndexManager::attach(SaturationAlgorithm* salg)
{
  CALL("IndexManager::attach");

  request(GENERATING_SUBST_TREE);
}

Index* IndexManager::request(IndexType t)
{
  CALL("IndexManager::request");

  // optimization:
  // if the simplifyingContainer and the toBeSimplifiedContainer are the same (which is the case for Discount, Otter, and LRS, but not for Discott),
  // we don't need to use (and therefore maintain) separate copies of indices.
  // Note that we currently only use SIMPLIFICATION_SUBST_TREE for both of these containers.
  if(t == BW_SIMPLIFICATION_SUBST_TREE && _equalSimpContainers)
  {
    t = FW_SIMPLIFICATION_SUBST_TREE;
  }

  Entry e;
  if(_store.find(t,e)) {
    e.refCnt++;
  } else {
    e.index=create(t);
    e.refCnt=1;
  }
  _store.set(t,e);
  return e.index;
}

void IndexManager::release(IndexType t)
{
  CALL("IndexManager::release");

  // optimization, see IndexManager::request()
  if(t == BW_SIMPLIFICATION_SUBST_TREE && _equalSimpContainers)
  {
    t = FW_SIMPLIFICATION_SUBST_TREE;
  }

  Entry e=_store.get(t);

  e.refCnt--;
  if(e.refCnt==0) {
    if(t==GENERATING_SUBST_TREE) {
      _genLitIndex=0;
    }
    delete e.index;
    _store.remove(t);
  } else {
    _store.set(t,e);
  }
}

bool IndexManager::contains(IndexType t)
{
  CALL("IndexManager::contains");

  // optimization, see IndexManager::request()
  if(t == BW_SIMPLIFICATION_SUBST_TREE && _equalSimpContainers)
  {
    t = FW_SIMPLIFICATION_SUBST_TREE;
  }

  return _store.find(t);
}

/**
 * If IndexManager contains index @b t, return pointer to it
 *
 * The pointer can become invalid after return from the code that
 * has requested it through this function.
 */
Index* IndexManager::get(IndexType t)
{
  CALL("IndexManager::get");

  // optimization, see IndexManager::request()
  if(t == BW_SIMPLIFICATION_SUBST_TREE && _equalSimpContainers)
  {
    t = FW_SIMPLIFICATION_SUBST_TREE;
  }

  return _store.get(t).index;
}

/**
 * Provide index form the outside
 *
 * There must not be index of the same type from before.
 * The provided index is never deleted by the IndexManager.
 */
void IndexManager::provideIndex(IndexType t, Index* index)
{
  CALL("IndexManager::provideIndex");
  ASS(!_store.find(t));

  Entry e;
  e.index = index;
  e.refCnt = 1; //reference to 1, so that we never delete the provided index
  _store.set(t,e);
}

Index* IndexManager::create(IndexType t)
{
  CALL("IndexManager::create");

  Index* res;
  LiteralIndexingStructure* is;
  TermIndexingStructure* tis;

  static bool useConstraints = env.options->unificationWithAbstraction()!=Options::UnificationWithAbstraction::OFF;
  switch(t) {
  case GENERATING_SUBST_TREE:
    is=new LiteralSubstitutionTree(useConstraints);
    _genLitIndex=is;
    res=new GeneratingLiteralIndex(is);
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;
  case FW_SIMPLIFICATION_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new SimplifyingLiteralIndex(is);
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;
  case BW_SIMPLIFICATION_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new SimplifyingLiteralIndex(is);
    res->attachContainer(_alg->getToBeSimplifiedClauseContainer());
    break;

  case FW_SIMPLIFYING_UNIT_CLAUSE_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new UnitClauseLiteralIndex(is);
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;
  case GENERATING_UNIT_CLAUSE_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new UnitClauseLiteralIndex(is);
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;
  case GENERATING_NON_UNIT_CLAUSE_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new NonUnitClauseLiteralIndex(is);
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;

  case SUPERPOSITION_SUBTERM_SUBST_TREE:
    tis=new TermSubstitutionTree(useConstraints);
    res=new SuperpositionSubtermIndex(tis, _alg->getOrdering());
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;
  case SUPERPOSITION_LHS_SUBST_TREE:
    tis=new TermSubstitutionTree(useConstraints);
    res=new SuperpositionLHSIndex(tis, _alg->getOrdering(), _alg->getOptions());
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;

  case ACYCLICITY_INDEX:
    tis = new TermSubstitutionTree();
    res = new AcyclicityIndex(tis);
    res->attachContainer(_alg->getGeneratingClauseContainer());
    break;

  case BW_DEMODULATION_SUBTERM_SUBST_TREE:
    tis=new TermSubstitutionTree();
    res=new DemodulationSubtermIndex(tis);
    res->attachContainer(_alg->getToBeSimplifiedClauseContainer());
    break;
  case FW_DEMODULATION_LHS_SUBST_TREE:
//    tis=new TermSubstitutionTree();
    tis=new CodeTreeTIS();
    res=new DemodulationLHSIndex(tis, _alg->getOrdering(), _alg->getOptions());
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;

  case FW_SUBSUMPTION_CODE_TREE:
    res=new CodeTreeSubsumptionIndex();
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;

  case FW_SUBSUMPTION_SUBST_TREE:
    is=new LiteralSubstitutionTree();
//    is=new CodeTreeLIS();
    res=new FwSubsSimplifyingLiteralIndex(is);
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;

  case FW_REWRITE_RULE_SUBST_TREE:
    is=new LiteralSubstitutionTree();
    res=new RewriteRuleIndex(is, _alg->getOrdering());
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;

  case FW_GLOBAL_SUBSUMPTION_INDEX:
    res = new GroundingIndex(_alg->getOptions());
    res->attachContainer(_alg->getSimplifyingClauseContainer());
    break;

  default:
    INVALID_OPERATION("Unsupported IndexType.");
  }

  return res;
}
