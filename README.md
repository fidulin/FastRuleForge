# FastRuleForge
### Author: Filip Černý
(fufinblack@gmail.com)

### RuleForge
**FastRuleForge** is a mangling-rule generation tool, faster version of [RuleForge](https://github.com/...), developed by **Lucie Šírová**, **Viktor Rucký**, and **Radek Hranický**. 
This implementation is heavily inspired from their work.

---

### Jaro-Winkler Distance

Implementation of the Jaro-Winkler distance, present in this project, is based on code from Rosetta Code:  
https://rosettacode.org/wiki/Jaro-Winkler_distance  
Licensed under the **GNU Free Documentation License 1.3**.

---

# Prerequisites

To build and run FastRuleForge on **Ubuntu**, ensure you have the following:

- `g++` with C++14 support
- OpenMP (`libgomp1`)
- OpenCL development headers (`opencl-headers`, `ocl-icd-opencl-dev`)
- Make

This program uses OpenCL, which needs runtimes for your GPU (could be preinstalled):
- `nvidia-opencl-icd` for NVIDIA GPUs
- `intel-opencl-icd` for INTEL GPUs

and others for respective vendors

To run OpenCL on CPU (not recomended, but its used as fallback):
- `pocl-opencl-icd`

For any questions about this, do not hesitate to contact me via email.

# Usage
First make to compile than:

() brackets for optional, [] for compulsory

./fastruleforge [--i [input_file] --o [output_file]]
(--HAC (threshold) | --LF (threshold) | --MLF (threshold_main threshold_sec threshold_total) | --MDBSCAN (eps_1 eps_2 minPts) | --DBSCAN (eps_1 minPts) | --AP (iter lambda))
(--verbose) (--no-randomize) (--set-rules ['rules'])

examples:
```
make
./fastruleforge --i passwords.txt --o rules.rule --LF --verbose

./fastruleforge --i passwords.txt --o rules.rule --MDBSCAN 2 0.25 3 --set-rules '$Ddr['

./fastruleforge --i passwords.txt --o rules.rule
```
