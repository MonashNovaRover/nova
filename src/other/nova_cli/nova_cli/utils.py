"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
General utility functions for nova CLI. Includes fuzzy string matching
for suggestions.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        nova_cli
AUTHOR(S):      Felicity Matthews
CREATION:       06/07/2026
EDITED:         09/07/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from difflib import get_close_matches


def fuzzy_match(target, options, n=3, cutoff=0.6):
    """
    Find close matches to target string in options.

    Args:
        target: String to match
        options: List of possible matches
        n: Max number of suggestions
        cutoff: Similarity threshold (0-1)

    Returns:
        List of close matches
    """
    return get_close_matches(target, options, n=n, cutoff=cutoff)
