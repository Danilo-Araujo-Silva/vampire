/*
 * File MLMatcher2.hpp.
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
 * @file MLMatcher2.hpp
 * Defines class MLMatcher2 with methods
 * supporting multiliteral matching.
 */

#ifndef __MLMatcher2__
#define __MLMatcher2__

#include "Clause.hpp"
#include "Forwards.hpp"
#include "Lib/STL.hpp"

namespace Kernel {

using namespace Lib;


class MLMatcher2
{
  private:
    /**
     * Initializes the matcher to the given match problem.  TODO: describe the match problem and what the parameters mean.
     * The matcher will be in a valid (but unmatched) state.
     *
     * Preconditions:
     * - baseLits must have length baseLen
     * - alts must have length baseLen (for 0 <= bi < baseLen, the literal baseLits[bi] will be matched against the alternatives in the list alts[bi])
     * - All literals in 'alts' must appear in 'instance'.
     * - If resolvedLit is not null, multiset must be false. (Hypothesis; not 100% sure if the matching algorithm breaks in that case)
     */
    void init(Literal* baseLits[],
              unsigned baseLen,
              Clause* instance,
              LiteralList* alts[],
              Literal* resolvedLit,
              bool multiset);

  public:
    /**
     * Constructs an MLMatcher2 and puts it in an invalid state.
     */
    MLMatcher2();

    void init(Literal* baseLits[], unsigned baseLen, Clause* instance, LiteralList* alts[], bool multiset = false)
    {
      init(baseLits, baseLen, instance, alts, nullptr, multiset);
    }

    void init(Clause* base, Clause* instance, LiteralList* alts[], bool multiset = false)
    {
      init(base->literals(), base->length(), instance, alts, multiset);
    }

    void init(Literal* baseLits[], unsigned baseLen, Clause* instance, LiteralList* alts[], Literal* resolvedLit)
    {
      // NOTE: we need multiset matching for subsumption, but for subsumption resolution it is not necessary
      init(baseLits, baseLen, instance, alts, resolvedLit, resolvedLit == nullptr);
    }

    void init(Clause* base, Clause* instance, LiteralList* alts[], Literal* resolvedLit)
    {
      init(base->literals(), base->length(), instance, alts, resolvedLit);
    }

    ~MLMatcher2();

    /**
     * Finds the next match.
     * May only be called if the matcher is in a valid state.
     * Return value:
     * - True if a match was found. The matcher is now in a valid and matched state.
     * - False if no more matches are possible. The matcher is now in an invalid state.
     */
    bool nextMatch();

    /**
     * Returns the alts which are currently matched by some base literal.
     * May only be called in a matched state (i.e., after nextMatch() has returned true).
     * May only be called if the matcher was initialized with resolvedLit == nullptr.
     */
    v_unordered_set<Literal*> getMatchedAlts() const;
    void getMatchedAlts(v_unordered_set<Literal*>& outAlts) const;

    /**
     * Returns the variable bindings due to the current match.
     * May only be called in a matched state (i.e., after nextMatch() has returned true).
     */
    v_unordered_map<unsigned, TermList> getBindings() const;
    void getBindings(v_unordered_map<unsigned, TermList>& outBindings) const;

    // Disallow copy because the internal implementation still uses pointers to the underlying storage and it seems hard to untangle that.
    MLMatcher2(MLMatcher2 const&) = delete;
    MLMatcher2& operator=(MLMatcher2 const&) = delete;

    // Moving works by moving the pointer m_impl
    MLMatcher2(MLMatcher2&&) = default;
    MLMatcher2& operator=(MLMatcher2&&) = default;

  private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

  public:
    /// Helper function for compatibility to previous code. It works with a shared static instance of MLMatcher2::Impl.
    static bool canBeMatched(Literal* baseLits[], unsigned baseLen, Clause* instance, LiteralList* alts[], Literal* resolvedLit, bool multiset);

    /// Helper function for compatibility to previous code. It works with a shared static instance of MLMatcher2::Impl.
    static bool canBeMatched(Clause* base,                          Clause* instance, LiteralList* alts[], Literal* resolvedLit)
    {
      return canBeMatched(base->literals(), base->length(), instance, alts, resolvedLit, resolvedLit == nullptr);
    }
};


};

#endif /* __MLMatcher2__ */