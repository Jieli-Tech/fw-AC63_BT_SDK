@echo off

cd /d %~dp0

tm_triad_prev_trans.exe  tmall_triad.csv
license_to_auth_csv.exe  tmall_triad_trans.csv
del /Q tmall_triad_trans.csv

pause