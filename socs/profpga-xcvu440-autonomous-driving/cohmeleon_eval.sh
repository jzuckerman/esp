set -x
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.5 0.25 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.45 0.225 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.4 0.2 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.35 0.175 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.3 0.15 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.25 0.125 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.2 0.1 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.15 0.075 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.1 0.05 0
./auton_driving.exe cfg_auton_driving_0.txt learn cfg 0.05 0.025 0
rm *.csv
./auton_driving.exe cfg_auton_driving_1.txt none cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt llc cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt recall cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt full cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt rand cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt design cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt manual cfg 0 0 0
./auton_driving.exe cfg_auton_driving_1.txt learn cfg 0 0 0
