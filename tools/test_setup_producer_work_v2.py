"""Run the frozen thirteen contract tests against the v2 verifier."""
import test_setup_producer_work as suite
import verify_setup_producer_work_v2 as verify

if __name__ == '__main__':
    suite.verify = verify
    suite.main()
