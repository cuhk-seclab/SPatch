# SPatch
SPatch is a tool to identify safe patches in C/C++ software using DSE and fine-grained patch analysis. Through comprehensively identifying safe patches, it finally harvests a substantial number of security patches.

File Organization
```tex
|--main: the source code of our tool.
  |--fetchCommit.py: the main function of SPatch that invokes other components.
  |--GetDiff.java: generating edit actions using Gumtree.
  |--split/split.py: generating CUs and testing their compatibility (Section 4.2).
  |--slice.sc: intra-procedural slicing (Section 4.3.1).
  |--split/z3helper.py: comparing symbolic expressions using z3 (Section 4.3.4).
  |--DSE_juxta: our implementation of DSE in Section 4.3.2.
  |--Others: dependency code, do not remove them.
|--artifacts: the complementary results and case studies.
  |--case study: the 100 safe patches used in our case study (Section 3.1).
  |--sspatch: the not ported safe security patches in downstream.
    |--lua-redis.txt: the safe security patches commited in lua but have not been ported to redis.
    |--openssl-ProtonVPN: the safe security patches commited in openssl but have not been ported to ProtonVPN. We did not discuss ProtonVPN in paper due to page limits.
|--evaluation: sample safe patches produced by SPatch.
  |--libarchieve: sample safe patches identified in libarchieve.
  |--lua: sample safe patches identified in lua.
  |--linux: sample safe patches identified in linux.

```

# Setup
SPatch requires python 3.8.1 (3.x versions should be OK), mlta, Joern, Gumtree. The experiments were conducted in Debian but other linux mechines should be OK.

# Usage
Use the following arugments to run the tool:
```tex
python3 fetchCommit.py [-p] [-u] [-c]
-p: Upstream program name (e.g., linux). We use it as path name to store evaluation results.
-u: The directory to upstream program.
-c: The commit from which ID SPatch starts the analysis
```
