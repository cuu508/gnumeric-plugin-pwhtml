import os
import shutil
import subprocess
import unittest
import tempfile

HERE = os.path.dirname(__file__)

class EscapingTestCase(unittest.TestCase):

    def _convert(self, filename):
        temp_path = tempfile.mkdtemp()
        in_path = os.path.join(HERE, "test-data", filename)
        out_path = os.path.join(temp_path, "result.pwhtml")
        try:
            retcode = subprocess.call(["ssconvert", in_path, out_path])
            assert retcode == 0
            result = open(out_path).read()
        finally:
            shutil.rmtree(temp_path)

        return result


    def test_quotes(self):
        r = self._convert("quotes.xls")
        # quotes should be replaced with &quot; in attributes
        assert 'raw="Bar &quot;Baz&quot;"' in r
        # but left alone in text
        assert 'Bar "Baz"' in r

if __name__ == '__main__':
    unittest.main()
