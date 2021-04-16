<project xmlns="com.autoesl.autopilot.project" name="myproject_prj" top="myproject">
    <libraryPaths/>
    <Simulation argv="">
        <SimFlow name="csim" ldflags="" mflags="" setup="false" optimizeCompile="false" clean="false"/>
    </Simulation>
    <includePaths xmlns=""/>
    <libraryFlag xmlns=""/>
    <files xmlns="">
        <file name="../../tb_data" sc="0" tb="1" cflags="  -Wno-unknown-pragmas"/>
        <file name="../../firmware/weights" sc="0" tb="1" cflags="  -Wno-unknown-pragmas"/>
        <file name="../../myproject_test.cpp" sc="0" tb="1" cflags=" -I/home/davide/Repos/esp/esp-ml/accelerators/vivado_hls/autoenc/hls4ml/nnet_utils  -std=c++0x -Wno-unknown-pragmas"/>
        <file name="firmware/myproject.cpp" sc="0" tb="false" cflags="-I/home/davide/Repos/esp/esp-ml/accelerators/vivado_hls/autoenc/hls4ml/nnet_utils -std=c++0x"/>
    </files>
    <solutions xmlns="">
        <solution name="solution1" status=""/>
    </solutions>
</project>

