/**
 *  @copyright Copyright (C) Michigan State University, MIT Software license
 *  @date 2023.
 *
 *  @file testing.hpp
 *  @brief A simple C++ unit-test managing system for CSE 232 at Michigan State University
 **/

#ifndef CSE232_UNIT_TESTING
#define CSE232_UNIT_TESTING

#include <sstream>
#include <string>
#include <vector>

namespace cse232 {

  //
  // ---------------------- Helper functions ----------------------
  //

  /// Return the first position found for any of a set of substring tests
  /// (or std::string::npos if none are found).
  template <typename... Ts>
  size_t find_any_of(
    const std::string test_str,
    size_t start_pos,
    std::string test1,
    Ts... tests)
  {
    size_t pos1 = test_str.find(test1, start_pos);
    if constexpr (sizeof...(Ts) == 0) return pos1;
    else {
      size_t pos2 = find_any_of(test_str, start_pos, tests...);
      if (pos1 == std::string::npos) return pos2;
      if (pos2 == std::string::npos) return pos1;
      return std::min(pos1, pos2);
    }
  }

  /// Remove leading and training whitespace from a string.
  std::string TrimWhitespace(std::string str) {
    size_t whitespace = 0;
    while (whitespace < str.size() && str[whitespace] == ' ') ++whitespace;
    if (whitespace) str.erase(0,whitespace);
    while (str.back() == ' ') str.pop_back();
    return str;
  }

  /// Flip the provided comparator to the opposite (so "<" becomes ">=").
  std::string FlipComparator(std::string in) {
    if (in == "==") return "!=";
    if (in == "!=") return "==";
    if (in == "<")  return ">=";
    if (in == "<=") return ">";
    if (in == ">")  return "<=";
    if (in == ">=") return "<";
    return "";
  }

  //
  // ---------------------- Main Unit Testing Class ----------------------
  //

  /// Manage a set of unit tests.
  struct UnitTester {
    // Information about the results of an individual check.  E.g.: CHECK(value == x+5);
    struct CheckInfo {
      std::string test = "";       // The original test as it was passed in (e.g., "value == x+5")
      std::string lhs = "";        // Representation of the left side of a test (e.g., "value")
      std::string rhs = "";        // Representation of the right side of a test (e.g., "x+5")
      std::string lhs_value = "";  // Representation of the value on the left (e.g., "20")
      std::string rhs_value = "";  // Representation of the value on the right (e.g., "21", if x=16)
      std::string comparator = ""; // Which comparison was used here?  (e.g., "==")
      bool passed = false;         // Was this check successful?
      bool resolved = false;       // Are we done performing this check?
      std::string message = "";    // Extra message on failure (e.g., "Grade assessments do not align.")
      std::string filename = "";   // What file is this check found in?
      size_t line_num = 0;         // What line number is this check on?
    };

    // Information about an entire test case.
    struct CaseInfo {
      std::string name;            // Unique name for this test case.
      double points = 0.0;         // Number of points this test case is worth.
      bool hidden = false;         // Should this test case be seen by students?

      std::vector<CheckInfo> checks;
    };

    // The set of all test cases being conducted.
    std::vector<CaseInfo> test_cases;

    // What output should we do?
    enum class Detail {
      SILENT = 0,   // No output, (internal use only).
      SUMMARY,      // Output only a final summary of number of cases passed/failed.
      NORMAL,       // Provide summary of passed cases and details on failed cases.
      VERBOSE,      // Provide details all along, including passed cases.
      DEBUG         // Full output of all details (including parsing), pass or fall.
    };
    Detail hidden_detail = Detail::SUMMARY;
    Detail public_detail = Detail::NORMAL;

    // Destructor will indicate that the unit testing is done an any summary data need to be printed.
    ~UnitTester() {
      // Loop through test cases.
      double total_points = 0.0;
      double earned_points = 0.0;
      for (auto & test_case : test_cases) {
        // Scan through individual checks for this case.
        size_t passed_count = 0;
        size_t total_count = 0;
        for (auto & check : test_case.checks) {
          ++total_count;
          if (check.passed) ++passed_count;
        }

        // Print the summary information:
        total_points += test_case.points;
        std::stringstream out;
        out << test_case.name << " : passed " << passed_count << " of " << total_count << " checks; ";
        if (passed_count == total_count) {
          out << test_case.points << " points." << std::endl;
          earned_points += test_case.points;
        } else {
          out << "0.0 points." << std::endl;
        }
        if ((test_case.hidden && hidden_detail >= Detail::SUMMARY) ||
            (!test_case.hidden && public_detail >= Detail::SUMMARY))
        {
          std::cout << out.str();
        }
      }

      std::cout << "\nFinal Score: " << earned_points << " / " << total_points << std::endl;
    }

    // Get the current open test case; create a default case if none exist.
    CaseInfo & GetCase() {
      // If we do not have a case, added a default one (with a warning.)
      if (test_cases.size() == 0) {
        std::cerr << "Added test case `DEFAULT`" << std::endl;
        test_cases.push_back(CaseInfo{"DEFAULT"});
      }
      return test_cases.back();
    }

    // Get the current open check in the current case.
    CheckInfo & GetCheck() { return GetCase().checks.back(); }

    // A proxy class to store partial (LHS) data from a unit test, while preserving type.
    template <typename T>
    struct UnitTestProxy {
      CheckInfo & check;
      const T & lhs_v;
      UnitTestProxy(CheckInfo & _check, const T & _value)
        : check(_check), lhs_v(_value)
      {
        // Store the provided lhs valse in the check.
        std::stringstream ss;
        ss << lhs_v;
        check.lhs_value = ss.str();

        // If the test does not have a RHS, convert LHS to a bool and stop here.
        if (check.rhs == "") {
          if constexpr (std::is_convertible_v<T, bool>) {
            check.passed = static_cast<bool>(lhs_v);
          }
          else if constexpr (std::is_convertible_v<T, std::string>) {
            check.passed = static_cast<std::string>(lhs_v).size();
          }
          else {
            std::cerr << "Internal error (line " << check.line_num << " of " << check.filename
		              << "): CHECK cannot be resolved to a bool." << std::endl;
            check.passed = false;
          }
          check.resolved = true;
        }
      }

      template <typename T2> 
      bool Resolve(bool result, const T2 & rhs_v) {
        // Store the right-hand-side value in the check.
        std::stringstream ss;
        ss << rhs_v;
        check.rhs_value = ss.str();

        check.passed = result;
        check.resolved = true;
        return check.passed;
      }

      template <typename T2> bool operator==(const T2 & rhs_v) { return Resolve(lhs_v == rhs_v, rhs_v); }
      template <typename T2> bool operator!=(const T2 & rhs_v) { return Resolve(lhs_v != rhs_v, rhs_v); }
      template <typename T2> bool operator< (const T2 & rhs_v) { return Resolve(lhs_v < rhs_v, rhs_v); }
      template <typename T2> bool operator<=(const T2 & rhs_v) { return Resolve(lhs_v <= rhs_v, rhs_v); }
      template <typename T2> bool operator> (const T2 & rhs_v) { return Resolve(lhs_v > rhs_v, rhs_v); }
      template <typename T2> bool operator>=(const T2 & rhs_v) { return Resolve(lhs_v >= rhs_v, rhs_v); }
    };

    
    CaseInfo & AddTestCase(const std::string & name, double points) {
      test_cases.push_back(CaseInfo{name, points});
      return test_cases.back();
    }

    template <typename... Ts>
    void SetMessage(Ts &&... args) {
      std::stringstream ss;
      if constexpr (sizeof...(Ts)) {
        (ss << ... << args);
      }
      GetCheck().message = ss.str();
    }

    /// Scan the check that's about to happen to make sure the test is legal and
    /// to store the two sides.
    void SetupCheck(const std::string & test, size_t line_num, const std::string & filename) {
      // For the moment we are allowing only a single comparison and no && or ||
      if (find_any_of(test, 0, "&&", "||") != std::string::npos) {
        std::cerr << "Internal error (line " << line_num << " of " << filename
		              << "): Unit test checks do not allow \"&&\" or \"||\"." << std::endl;
	      exit(1);
      }

      // Make sure there is a single comparator.
      auto comp_pos = find_any_of(test, 0, "==", "!=", "<", "<=", ">", ">=");

      // Make sure we don't have a second comparator.
      if (comp_pos != std::string::npos &&
          find_any_of(test, comp_pos+2, "==", "!=", "<", "<=", ">", ">=") != std::string::npos) {
        std::cerr << "Internal error (line " << line_num << " of " << filename
	                << "): Unit test checks cannot have more than one comparison." << std::endl;
        exit(1);
      }

      // Determine which comparison operator we are working with (if any) and the terms being compared.
      CheckInfo check_info;
      check_info.test = test; // Save the original test.
      check_info.filename = filename;
      check_info.line_num = line_num;
      if (comp_pos != std::string::npos) {
        check_info.comparator += test[comp_pos];
        if (test[comp_pos+1] == '=') check_info.comparator += "=";

        // Identify the left and right hand sides
        check_info.lhs = TrimWhitespace(test.substr(0,comp_pos));
        check_info.rhs = TrimWhitespace(test.substr(comp_pos+check_info.comparator.size()));
      }
      else {
        check_info.lhs = test;
      }

      // Insert this check's information into the current test case.
      GetCase().checks.push_back(check_info);
    }

    // Tests will be started with the < operator.
    template <typename T>
    UnitTestProxy<T> operator<(const T & in) {
      return UnitTestProxy<T>(GetCheck(), in);
    }

    // Print the results for the latest check performed.
    void PrintCheckResults() {
      CaseInfo & case_info = GetCase();
      CheckInfo & check = GetCheck();
      Detail detail = case_info.hidden ? hidden_detail : public_detail;
      switch (detail) {
      case Detail::NORMAL:
        if (check.passed) break; // Only print extra information for failed cases.
        [[fallthrough]];
      case Detail::VERBOSE:
        std::cout << "CHECK ('" << check.filename << ", line " << check.line_num << ") "
                  << (check.passed ? "[PASSED]:" : "[FAILED!]: ") << check.test << std::endl;
        if (!check.passed && check.rhs != "") {
          std::cout << check.lhs_value << " " << FlipComparator(check.comparator) << " "
                    << check.rhs_value << std::endl;
          // If this failed check had a message with it, print the message.
          if (check.message.size()) {
            std::cout << check.message << std::endl;
          }
          std::cout << std::endl; // Skip a line.
        }
        break;
      case Detail::DEBUG:
        std::cout << "ORIGINAL  : " << check.test << std::endl;
        std::cout << "lhs       : " << check.lhs << std::endl;
        std::cout << "rhs       : " << check.rhs << std::endl;
        std::cout << "lhs_value : " << check.lhs_value << std::endl;
        std::cout << "rhs_value : " << check.rhs_value << std::endl;
        std::cout << "comparator: " << check.comparator << std::endl;
        std::cout << "passed    : " << check.passed << std::endl;
        std::cout << "resolved  : " << check.resolved << std::endl;
        std::cout << "message   : " << check.message << std::endl;
        std::cout << "filename  : " << check.filename << std::endl;
        std::cout << "line_num  : " << check.line_num << std::endl;
        std::cout << std::endl;
        break;
      default:
        break;
      }
    }
  };

  /// Create a singleton unit tester.
  static UnitTester & GetUnitTester() {
    static UnitTester ut;
    return ut;
  }
  
}


//
// ---------------------- Unit Testing Macros ----------------------
//


/// Start a new test case with the given number of points.
#define TEST_CASE(NAME, POINTS) {                    \
    auto & unit_tester = cse232::GetUnitTester();    \
    unit_tester.AddTestCase(NAME, POINTS);           \
  }

/// Start a new test case with the given number of points.
#define HIDDEN_TEST_CASE(NAME, POINTS) {                 \
    auto & unit_tester = cse232::GetUnitTester();	       \
    unit_tester.AddTestCase(NAME, POINTS).hidden = true; \
  }

/// A single CHECK within a unit test.  If additional arguments are included
/// they are printed when if test fails.
#define CHECK(TEST, ...) {		                          \
    auto & unit_tester = cse232::GetUnitTester();       \
    unit_tester.SetupCheck(#TEST, __LINE__, __FILE__);  \
    unit_tester.SetMessage(__VA_ARGS__);                \
    unit_tester < TEST;                                 \
    unit_tester.PrintCheckResults();                    \
  }

#define SET_HIDDEN_DETAIL(LEVEL) \
  cse232::GetUnitTester().hidden_detail = cse232::UnitTester::Detail::LEVEL
#define SET_PUBLIC_DETAIL(LEVEL) \
  cse232::GetUnitTester().public_detail = cse232::UnitTester::Detail::LEVEL

#endif