# Chaste_Relax11

Randy (and Claude's) attempt to reproduce the 11-cell relaxation model/results in Chaste. This repo contains the essential bits of the model and results that would normally appear in the larger Chaste build directory structure.

The model is at https://github.com/rheiland/Chaste_Relax11/blob/main/projects/Relax11/apps/src/Relax11.cpp

The simple .csv output is at https://github.com/rheiland/Chaste_Relax11/blob/main/chaste_build/testoutput/Relax11/results_from_time_0/cells.csv
and we provide a plotting script to plot the 11 cells over time:
```
(base) M1P~/git/Chaste_Relax11/chaste_build/testoutput/Relax11/results_from_time_0$ python plot_csv.py cells.csv 
```
that results in (using the default 15.0 repulsion force):

<img src=relax_90pct.png width=500>

If we choose to use 5x this duration to reach 90% width, then duration time=5.2, resulting in a cell cycle (growth rate) = 1/5.2=0.192. But this rate cannot be correct (way too large; cycle duration way too small). So how is my model not correct - is it force calculation?

Lowering the repulsion force to 5.0 (as is done in the latest committed version) does indeed increase the time to reach 90% width, but it seems that it would just slow down the expansion of the 1000 or 10000 monolayer, resulting in overlap in the interior cells (as happens with PhysiCell).
