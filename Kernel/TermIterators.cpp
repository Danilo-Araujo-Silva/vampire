
/*
 * File TermIterators.cpp.
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
 * @file TermIterators.cpp
 * Implements several iteratorn over terms.
 */


#include "Debug/Tracer.hpp"

#include "Term.hpp"
#include "Signature.hpp"
#include "TermIterators.hpp"
#include "ApplicativeHelper.hpp"
#include "Lib/Deque.hpp"

namespace Kernel
{

typedef ApplicativeHelper AH;

/**
 * True if there exists next variable
 */
bool VariableIterator::hasNext()
{
  CALL("VariableIterator::hasNext");
  if(_stack.isEmpty()) {
    return false;
  }
  if(!_used && _stack.top()->isVar()) {
    return true;
  }
  while(!_stack.isEmpty()) {
    const TermList* t=_stack.pop();
    if(_used && t->isVar()) {
      _used=false;
      t=t->next();
    }
    if(t->isEmpty()) {
	continue;
    }
    if(t->isVar()) {
      ASS(!_used);
      _stack.push(t);
      return true;
    }
    _stack.push(t->next());
    ASS(t->isTerm());
    const Term* trm=t->term();
    if(!trm->shared() || !trm->ground()) {
      _stack.push(trm->args());
    }
  }
  return false;
}


///////////////////////////////////////////


/**
 * True if there exists next subterm
 */
bool SubtermIterator::hasNext()
{
  CALL("SubtermIterator::hasNext");

  if(_stack->isEmpty()) {
    return false;
  }
  if(!_used) {
    return true;
  }
  _used=false;
  const TermList* t=_stack->pop();
  pushNext(t->next());
  if(t->isTerm()) {
    pushNext(t->term()->args());
  }
  return !_stack->isEmpty();
}

/**
 * Skip subterms of the term just returned by the @b next function
 *
 * Must be called after the @b next function before the @b hasNext
 * function is called again.
 */
void SubtermIterator::right()
{
  CALL("SubtermIterator::right");
  ASS(_stack->isNonEmpty());
  ASS(_used);

  _used=false;
  const TermList* t=_stack->pop();
  pushNext(t->next());

  //we did here the same as in the hasNext function, we only didn't call
  //the pushNext function on arguments of t
}


bool UnstableSubTermIt::hasNext()
{
  CALL("UnstableSubTermIt::hasNext");

  static TermStack args;
  args.reset();
  TermList head;
  
  if(_next){ return true; }
  while(!_stack.isEmpty()){
    Term* t = _stack.pop();
    ASS(AH::isApp(t));
    AH::getHeadAndArgs(t, head, args);
    ASS(args.size());
    if(head.isVar() && !isSafe(args)){
      _next = t;
    } else if(AH::isComb(head) && !AH::isUnderApplied(head, args.size()) && unstable(t)){
      _next = t;
    } else {
      while(!args.isEmpty()){
        TermList tl = args.pop();
        if(!tl.isVar() && AH::isApp(tl.term()) && !tl.term()->ground()){
          _stack.push(tl.term());
        }
      }
    }
    if(_next) { return true; }
  }
  return false;  
}

bool UnstableSubTermIt::isSafe(TermStack& args)
{
  CALL("UnstableSubTermIt::isSafe");

  for(unsigned i = 0; i < args.size(); i++){
    TermList head = AH::getHead(args[i]);
    if(head.isVar() || AH::isComb(head)){
      return false;
    }
  }
  return true;
}

bool UnstableSubTermIt::unstable(Term* t)
{
  CALL("UnstableSubTermIt::unstable");
  
  TermStack args;
  TermList head;
  
  AH::getHeadAndArgs(t, head, args);
  while(!args.isEmpty()){
    TermList t = args.pop();
    if(t.isVar()){
      return true;
    }
    head = AH::getHead(t);
    if(AH::isComb(head)){
      TermStack args2;
      AH::getHeadAndArgs(t, head, args2);
      while(!args2.isEmpty()){
        args.push(args2.pop());
      }
    }
  }
  return false;
}

///////////////////////////////////////////

bool StableVarIt::hasNext()
{
  CALL("StableVarIt::hasNext");

  static TermStack args;
  args.reset();
  TermList head;
  
  if(!_next.isEmpty()){ return true; }
  while(!_stack.isEmpty()){
    TermList t = _stack.pop();
    if(t.isVar()){
      _next = t;
    } else {
      AH::getHeadAndArgs(t, head, args);
      if(head.isVar()){
        _next = t;
      }
      while(!args.isEmpty()){
        TermList tl = args.pop();
        if(tl.isVar() || (!tl.term()->ground() && !_unstableTerms->find(tl.term()))){
          _stack.push(tl);
        }
      }
    }
    if(!_next.isEmpty()) { return true; }
  }
  return false;  

}


///////////////////////////////////////////

/*bool UnappliedTermVarIterator::hasNext()
{
  CALL("UnappliedTermVarIterator::hasNext");

  if(!_next.isEmpty()){ return true; }
  while(!_stack.isEmpty()){
    const Term* t = _stack.pop();
    ASS(AH::isApp(t));
    const TermList* tl = t->nthArgument(2);
    if(AH::isApp(tl) && !tl->term()->ground()){
      _stack.push(tl->term());
    }
    tl = t->nthArgument(3);
    if(tl->isVar()){
      _next = *tl;
      return true;
    }
    if(AH::isApp(tl) && !tl->term()->ground()){
      _stack.push(tl->term());
    }
  }
  return false;
}

///////////////////////////////////////////

bool AppliedVarIterator::hasNext()
{
  CALL("AppliedVarIterator::hasNext");

  static Deque<TermList> args;
  TermList head;

  if(!_next.isEmpty()){ return true; }
  while(!_stack.isEmpty()){
    Term* t = _stack.pop();
    ASS(AH::isApp(t));
    args.reset();
    AH::getHeadAndArgs(t, head, args);
    ASS(args.size());
    if(head.isVar()){
      _next = TermList(t);
    }
    while((_next.isEmpty() || _under) && !args.isEmpty()){
      TermList tl = args.pop_back();
      if(!tl.isVar() && AH::isApp(tl.term()) && !tl.term()->ground()){
        _stack.push(tl.term());
      }
    }
    if(!_next.isEmpty()){ return true; }
  }
  return false;
}*/

///////////////////////////////////////////

/**
 * True if there exists next subterm
 */
bool PolishSubtermIterator::hasNext()
{
  CALL("PolishSubtermIterator::hasNext");

  if(_stack.isEmpty()) {
    return false;
  }
  if(!_used) {
    return true;
  }
  _used=false;
  const TermList* t=_stack.pop();
  pushNext(t->next());
  return !_stack.isEmpty();
}

/**
 * Return the next non-variable subterm.
 * @since 04/05/2013 Manchester
 * @author Andrei Voronkov
 */
TermList NonVariableIterator::next()
{
  CALL("NonVariableIterator::next");

  Term* t = _stack.pop();
  _added = 0;
  Term::Iterator ts(t);
  for (const TermList* ts = t->args();!ts->isEmpty();ts = ts->next()) {
    if (ts->isTerm()) {
      _stack.push(const_cast<Term*>(ts->term()));
      _added++;
    }
  }
  return TermList(t);
} // NonVariableIterator::next

/**
 * Skip all subterms of the terms returned by the last call to next()
 * @since 04/05/2013 Manchester
 * @author Andrei Voronkov
 */
void NonVariableIterator::right()
{
  CALL("NonVariableIterator::right");

  while (_added > 0) {
    _added--;
    _stack.pop();
  }
} // NonVariableIterator::right

/**
 * Return the next non-variable subterm that is not a type argument.
 * @since 20/06/2019 Manchester
 * @author Ahmed Bhayat
 */
TermList NonVariableNonTypeIterator::next()
{
  CALL("NonVariableNonTypeIterator::next");

  Term* t = _stack.pop();
  TermList* ts;
  _added = 0;  
  Signature::Symbol* sym;
  if (t->isLiteral()) {
    sym = env.signature->getPredicate(t->functor());
  } else{
    sym = env.signature->getFunction(t->functor());
  }
  unsigned taArity; 
  unsigned arity;
  
  if(t->isLiteral() && static_cast<Literal*>(t)->isEquality()){
    taArity = 0;
    arity = 2;
  } else {
    taArity = sym->typeArgsArity();
    arity = sym->arity();
  }

  for(unsigned i = taArity; i < arity; i++){
    ts = t->nthArgument(i);
    if (ts->isTerm()) {
      _stack.push(const_cast<Term*>(ts->term()));
      _added++;
    }
  }
  return TermList(t);
}

/**
 * Skip all subterms of the terms returned by the last call to next()
 * @since 04/05/2013 Manchester
 * @author Andrei Voronkov
 */
void NonVariableNonTypeIterator::right()
{
  CALL("NonVariableNonTypeIterator::right");

  while (_added > 0) {
    _added--;
    _stack.pop();
  }
} // NonVariableIterator::right

/**
 * True if there exists next non-variable subterm
 */
// bool NonVariableIterator::hasNext()
// {
//   CALL("NonVariableIterator::hasNext");

//   if(_stack.isEmpty()) {
//     return false;
//   }
//   ASS(_stack.top()->isTerm());
//   if(!_used) {
//     return true;
//   }
//   _used=false;
//   const TermList* t=_stack.pop();
//   pushNextNonVar(t->next());
//   pushNextNonVar(t->term()->args());
//   return !_stack.isEmpty();
// }

// /**
//  * Skip subterms of the term just returned by the @b next function
//  *
//  * Must be called after the @b next function before the @b hasNext
//  * function is called again.
//  */
// void NonVariableIterator::right()
// {
//   CALL("NonVariableIterator::right");
//   ASS(_stack.isNonEmpty());
//   ASS(_used);

//   _used=false;
//   const TermList* t=_stack.pop();
//   pushNextNonVar(t->next());

//   //we did here the same as in the hasNext function, we only didn't call
//   //the pushNextNonVar function on arguments of t
// }

// void NonVariableIterator::pushNextNonVar(const TermList* t)
// {
//   while(t->isVar()) {
//     t=t->next();
//   }
//   if(!t->isEmpty()) {
//     ASS(t->isTerm());
//     _stack.push(t);
//   }
// }


///////////////////////////////////////////

/**
 * True if there exists another disagreement between the two
 * terms specified in the constructor.
 */
bool DisagreementSetIterator::hasNext()
{
  CALL("DisagreementSetIterator::hasNext");
  ASS(_stack.size()%2==0);

  if(!_arg1.isEmpty()) {
    return true;
  }
  if(_stack.isEmpty()) {
    return false;
  }
  TermList* ss; //t1 subterms
  TermList* tt; //t2 subterms
  while(!_stack.isEmpty()) {
    tt=_stack.pop();
    ss=_stack.pop();
    if(!ss->next()->isEmpty()) {
      _stack.push(ss->next());
      _stack.push(tt->next());
    }
    if(!_disjunctVariables && ss->sameContent(tt)) {
      //if content is the same, neighter weightDiff nor varDiffs would change
      continue;
    }
    if(TermList::sameTopFunctor(*ss,*tt)) {
      ASS(ss->isTerm());
      ASS(tt->isTerm());
      if(ss->term()->arity()) {
	_stack.push(ss->term()->args());
	_stack.push(tt->term()->args());
      }
    } else {
      _arg1=*ss;
      _arg2=*tt;
      return true;
    }
  }
  return false;
}


///////////////////////////////////////////

/**
 * Build an iterator over symbols of t.
 * @since 26/05/2007 Manchester
 */
TermFunIterator::TermFunIterator (const Term* t)
  : _stack(64)
{
  CALL("TermFunIterator::TermFunIterator");

  _hasNext = true;
  _next = t->functor();
  _stack.push(t->args());
} // TermFunIterator::TermFunIterator


/**
 * True if there the next function.
 * @since 26/05/2007 Manchester
 */
bool TermFunIterator::hasNext ()
{
  CALL("TermFunIterator::hasNext");

  if (_hasNext) {
    return true;
  }

  while (_stack.isNonEmpty()) {
    const TermList* ts = _stack.pop();
    if (ts->isEmpty()) {
      continue;
    }
    _stack.push(ts->next());
    if (ts->isVar()) {
      continue;
    }
    _hasNext = true;
    const Term* t = ts->term();
    _next = t->functor();
    _stack.push(t->args());
    return true;
  }
  return false;
} // TermFunIterator::hasNext


/**
 * Return the next symbol.
 * @since 26/05/2007 Manchester
 */
unsigned TermFunIterator::next ()
{
  CALL("TermFunIterator::hasNext");

  ASS(_hasNext);

  _hasNext = false;
  return _next;
} // TermFunIterator::next


///////////////////////////////////////////

/**
 * Build an iterator over variables of t.
 */
TermVarIterator::TermVarIterator (const Term* t)
  : _stack(64)
{
  CALL("TermVarIterator::TermVarIterator");
  //TODO update for two var lits?
  _stack.push(t->args());
} // TermVarIterator::TermVarIterator

/**
 * Build an iterator over variables of t.
 * @since 06/01/2004 Manchester
 * @since 26/05/2007 Manchester, reimplemented
 */
TermVarIterator::TermVarIterator (const TermList* ts)
  : _stack(64)
{
  CALL("TermVarIterator::TermVarIterator");
  _stack.push(ts);
} // TermVarIterator::TermVarIterator


/**
 * True if there the next variable.
 * @since 06/01/2004 Manchester
 * @since 26/05/2007 Manchester, reimplemented for new datastructures
 */
bool TermVarIterator::hasNext ()
{
  CALL("TermVarIterator::hasNext");

  while (_stack.isNonEmpty()) {
    const TermList* ts = _stack.pop();
    if (ts->isEmpty()) {
      continue;
    }
    _stack.push(ts->next());
    if (ts->isVar()) {
      _next = ts->var();
      return true;
    }
    _stack.push(ts->term()->args());
  }
  return false;
} // TermVarIterator::hasNext


/**
 * Return the next variable.
 * @since 06/01/2004 Manchester
 * @since 26/05/2007 Manchester, reimplemented for new datastructures
 */
unsigned TermVarIterator::next ()
{
  CALL("TermVarIterator::next");
  return _next;
} // TermVarIterator::next


///////////////////////////////////////////



}
