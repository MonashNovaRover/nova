"""Tests for utils.py"""
import pytest

from nova_cli.utils import fuzzy_match


class TestFuzzyMatch:
    """Tests for fuzzy_match function"""

    def test_exact_match(self):
        """Exact match is returned"""
        options = ["science", "drive", "auto"]
        result = fuzzy_match("science", options)
        assert "science" in result

    def test_close_match(self):
        """Close matches are returned"""
        options = ["science_bringup", "drive_bringup", "auto_bringup"]
        result = fuzzy_match("scence_bringup", options)  # Typo
        assert "science_bringup" in result

    def test_no_match(self):
        """No matches returns empty list"""
        options = ["science", "drive", "auto"]
        result = fuzzy_match("completely_different", options)
        assert result == []

    def test_max_results(self):
        """Respects n parameter for max results"""
        options = ["test1", "test2", "test3", "test4"]
        result = fuzzy_match("test", options, n=2)
        assert len(result) <= 2

    def test_cutoff_threshold(self):
        """Respects cutoff threshold"""
        options = ["science", "drive"]
        # High cutoff should exclude weak matches
        result = fuzzy_match("sci", options, cutoff=0.9)
        assert "science" not in result
