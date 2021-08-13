set -x
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.5 0.25 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.45 0.225 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.4 0.2 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.35 0.175 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.3 0.15 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.25 0.125 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.2 0.1 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.15 0.075 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.1 0.05 0
./comp_vision.exe cfg_comp_vision_0.txt learn cfg 0.05 0.025 0
rm *.csv
./comp_vision.exe cfg_comp_vision_1.txt none cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt llc cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt recall cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt full cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt rand cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt design cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt auto cfg 0 0 0
./comp_vision.exe cfg_comp_vision_1.txt learn cfg 0 0 0
