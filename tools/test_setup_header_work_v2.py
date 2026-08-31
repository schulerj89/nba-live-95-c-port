"""Run the frozen thirteen contract tests against the v2 verifier."""
import test_setup_header_work as suite
import verify_setup_header_work_v2 as verify

if __name__ == '__main__':
    suite.verify = verify
    suite.main()
