#!/bin/bash
# copyright (c) 2018 quackmore-ff@yahoo.com
#

echo "based on gen_misc.sh version 20150511"
echo ""

echo "Specify the COM port"

if [ -e env.sh ]; then
    comport=$(grep COMPORT env.sh | sed s/export\ COMPORT=//)
    if [ -z "$comport" ]; then
        echo "Enter the COM port: (default /dev/ttyUSB0)"
    else
        echo "Enter the COM port: (default $comport)"
    fi
fi

read input

if [ -z "$input" ]; then
    if [ -z "$comport" ]; then
        comport="/dev/ttyUSB0"
    fi
else
    comport=$input
fi

echo "COM port: $comport"
echo ""


echo "Specify the compiler path"

if [ -e env.sh ]; then
    cc_dir=$(grep CC_DIR env.sh | sed s/export\ CC_DIR=//)
    if [ -z "$cc_dir" ]; then
        echo "Enter the path:"
    else
        echo "Enter the path: (default $cc_dir)"
    fi
fi

read input

if [ -z "$input" ]; then
    if [ -z "$cc_dir" ]; then
        echo "ERROR: cannot accept an empty path ..."
        exit 1
    fi
else
    cc_dir=$input
fi

echo "CC path: $cc_dir"
echo ""

echo "Specify the SDK path"

if [ -e env.sh ]; then
    sdkdir=$(grep SDK_DIR env.sh | sed s/export\ SDK_DIR=//)
    if [ -z "$sdkdir" ]; then
        echo "Enter the path:"
    else
        echo "Enter the path: (default $sdkdir)"
    fi
fi

read input

if [ -z "$input" ]; then
    if [ -z "$sdkdir" ]; then
        echo "ERROR: cannot accept an empty path ..."
        exit 1
    fi
else
    sdkdir=$input
fi

echo "SDK path: $sdkdir"
echo ""

echo "Please follow below steps(1-5) to generate specific bin(s):"
echo "STEP 1: choose boot version(0=boot_v1.1, 1=boot_v1.2+, 2=none)"
echo "enter(0/1/2, default 1):"
read input

if [ -z "$input" ]; then
    boot=new
    user_start_addr=0x01000
elif [ $input == 0 ]; then
	boot=old
    user_start_addr=0x01000
elif [ $input == 1 ]; then
    boot=new
    user_start_addr=0x01000
else
    boot=none
    user_start_addr=0x00000
fi

echo "boot mode: $boot"
echo ""

echo "STEP 2: choose bin generate(0=eagle.flash.bin+eagle.irom0text.bin, 1=user1.bin, 2=user2.bin)"
echo "enter (0/1/2, default 1):"
read input

if [ -z "$input" ]; then
    if [ $boot == none ]; then
    	app=0
	echo "choose no boot before"
	echo "generate bin: eagle.flash.bin+eagle.irom0text.bin"
    else
	app=1
        echo "generate bin: user1.bin"
    fi
elif [ $input == 1 ]; then
    if [ $boot == none ]; then
    	app=0
	echo "choose no boot before"
	echo "generate bin: eagle.flash.bin+eagle.irom0text.bin"
    else
	app=1
        echo "generate bin: user1.bin"
    fi
elif [ $input == 2 ]; then
    if [ $boot == none ]; then
    	app=0
	echo "choose no boot before"
	echo "generate bin: eagle.flash.bin+eagle.irom0text.bin"
    else
    	app=2
    	echo "generate bin: user2.bin"
    fi
else
    if [ $boot != none ]; then
    	boot=none
	echo "ignore boot"
    fi
    app=0
    echo "generate bin: eagle.flash.bin+eagle.irom0text.bin"
fi

echo ""

echo "STEP 3: choose spi speed(0=20MHz, 1=26.7MHz, 2=40MHz, 3=80MHz)"
echo "enter (0/1/2/3, default 2):"
read input

if [ -z "$input" ]; then
    spi_speed=40
    spi_ff="40m"
    freqdiv=0
elif [ $input == 0 ]; then
    spi_speed=20
    spi_ff="20m"
    freqdiv=2
elif [ $input == 1 ]; then
    spi_speed=26.7
    spi_ff="26m"
    freqdiv=1
elif [ $input == 3 ]; then
    spi_speed=80
    spi_ff="80m"
    freqdiv=15
else
    spi_speed=40
    spi_ff="80m"
    freqdiv=0
fi

echo "spi speed: $spi_speed MHz"
echo ""

echo "STEP 4: choose spi mode(0=QIO, 1=QOUT, 2=DIO, 3=DOUT)"
echo "enter (0/1/2/3, default 0-QIO):"
read input

if [ -z "$input" ]; then
    spi_mode="qio"
    mode=0
elif [ $input == 0 ]; then
    spi_mode="qio"
    mode=0
elif [ $input == 1 ]; then
    spi_mode="quot"
    mode=1
elif [ $input == 2 ]; then
    spi_mode="dio"
    mode=2
elif [ $input == 3 ]; then
    spi_mode="dout"
    mode=3
else
    spi_mode="qio"
    mode=0
fi

echo "spi mode: $spi_mode"
echo ""

echo "STEP 5: choose spi size and map"
echo "    0 =  256KB (NO OTA)"
echo "    1 =  512KB (NO OTA)"
echo "    2 =  1MB (OTA ->  512KB +  512KB)"
echo "    3 =  2MB (OTA ->  512KB +  512KB)"
echo "    4 =  4MB (OTA ->  512KB +  512KB)"
echo "    5 =  2MB (OTA -> 1024KB + 1024KB)"
echo "    6 =  4MB (OTA -> 1024KB + 1024KB)"
echo "    7 =  8MB (OTA -> 1024KB + 1024KB)"
echo "    8 = 16MB (OTA -> 1024KB + 1024KB)"
echo "enter (0/1/2/3/4/5/6/8/9, default 4):"
read input

if [ -z "$input" ]; then
    spi_size_map=4
    spi_fs="32m"
    flash_init="0x3FB000 $sdkdir/bin/blank.bin 0x3FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x3FE000 $sdkdir/bin/blank.bin"
    flash_size=4096
    ld_ref=1024
    if [ $app == 2 ]; then
        user_start_addr=0x81000
    fi
    echo "flash size: 4MB"
    echo "ota map:  512KB + 512KB"
elif [ $input == 0 ]; then
    spi_size_map=0
    spi_fs="2m"
    flash_init="0x3B000 $sdkdir/bin/blank.bin 0x3C000 $sdkdir/bin/esp_init_data_default_v08.bin 0x3E000 $sdkdir/bin/blank.bin"
    flash_size=256
    echo "flash size: 256KB"
    echo "no ota"
elif [ $input == 1 ]; then
    spi_size_map=1
    spi_fs="4m"
    flash_init="0x7B000 $sdkdir/bin/blank.bin 0x7C000 $sdkdir/bin/esp_init_data_default_v08.bin 0x7E000 $sdkdir/bin/blank.bin"
    flash_size=512
    echo "flash size: 512KB"
    echo "no ota"
elif [ $input == 2 ]; then
    spi_size_map=2
    spi_fs="8m"
    flash_init="0xFB000 $sdkdir/bin/blank.bin 0xFC000 $sdkdir/bin/esp_init_data_default_v08.bin 0xFE000 $sdkdir/bin/blank.bin"
    flash_size=1024
    ld_ref=1024
    if [ $app == 2 ]; then
        user_start_addr=0x41000
    fi
    echo "flash size: 1MB"
    echo "ota map:  512KB + 512KB"
elif [ $input == 3 ]; then
    spi_size_map=3
    spi_fs="16m"
    flash_init="0x1FB000 $sdkdir/bin/blank.bin 0x1FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x1FE000 $sdkdir/bin/blank.bin"
    flash_size=2048
    ld_ref=1024
    if [ $app == 2 ]; then
        user_start_addr=0x81000
    fi
    echo "flash size: 2MB"
    echo "ota map:  512KB + 512KB"
elif [ $input == 4 ]; then
    spi_size_map=4
    spi_fs="32m"
    flash_init="0x3FB000 $sdkdir/bin/blank.bin 0x3FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x3FE000 $sdkdir/bin/blank.bin"
    flash_size=4096
    ld_ref=1024
    if [ $app == 2 ]; then
        user_start_addr=0x81000
    fi
    echo "flash size: 4MB"
    echo "ota map:  512KB + 512KB"
elif [ $input == 5 ]; then
    spi_size_map=5
    spi_fs="16m-c1"
    flash_init="0x1FB000 $sdkdir/bin/blank.bin 0x1FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x1FE000 $sdkdir/bin/blank.bin"
    flash_size=2048
    ld_ref=2048
    if [ $app == 2 ]; then
        user_start_addr=0x101000
    fi
    echo "flash size: 2MB"
    echo "ota map:  1024KB + 1024KB"
elif [ $input == 6 ]; then
    spi_size_map=6
    spi_fs="32m-c1"
    flash_init="0x3FB000 $sdkdir/bin/blank.bin 0x3FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x3FE000 $sdkdir/bin/blank.bin"
    flash_size=4096
    ld_ref=2048
    if [ $app == 2 ]; then
        user_start_addr=0x101000
    fi
    echo "flash size: 4MB"
    echo "ota map:  1024KB + 1024KB"
elif [ $input == 8 ]; then
    spi_size_map=8
    spi_fs="32m-c1"
    flash_init="0x7FB000 $sdkdir/bin/blank.bin 0x7FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x7FE000 $sdkdir/bin/blank.bin"
    flash_size=8192
    ld_ref=2048
    if [ $app == 2 ]; then
        user_start_addr=0x101000
    fi
    echo "flash size: 8MB"
    echo "ota map:  1024KB + 1024KB"
elif [ $input == 9 ]; then
    spi_size_map=9
    spi_fs="32m-c1"
    flash_init="0xFFB000 $sdkdir/bin/blank.bin 0xFFC000 $sdkdir/bin/esp_init_data_default_v08.bin 0xFFE000 $sdkdir/bin/blank.bin"
    flash_size=16384
    ld_ref=2048
    if [ $app == 2 ]; then
        user_start_addr=0x101000
    fi
    echo "flash size: 16MB"
    echo "ota map:  1024KB + 1024KB"
else
    spi_size_map=4
    spi_fs="32m"
    flash_init="0x3FB000 $sdkdir/bin/blank.bin 0x3FC000 $sdkdir/bin/esp_init_data_default_v08.bin 0x3FE000 $sdkdir/bin/blank.bin"
    flash_size=4096
    ld_ref=1024
    if [ $app == 2 ]; then
        user_start_addr=0x81000
    fi
    echo "flash size: 4MB"
    echo "ota map:  512KB + 512KB"
fi

flash_options=" write_flash -fm "$spi_mode" -fs "$spi_fs" -ff "$spi_ff" "

echo ""
echo "Generating bin options as env variables into ./env.sh"
echo "Exec '. env.sh' to set env variables"

echo 'export COMPILE='gcc > env.sh
echo 'export COMPORT='$comport >> env.sh
echo 'export CC_DIR='$cc_dir >> env.sh
check_path=$(echo $PATH | grep $cc_dir)
if [ -z "$check_path" ]; then
    echo 'export PATH=$PATH:'$cc_dir >> env.sh
fi
echo 'export SDK_DIR='$sdkdir >> env.sh
echo 'export BOOT='$boot >> env.sh
echo 'export APP='$app >> env.sh
echo 'export SPI_SPEED='$spi_speed >> env.sh
echo 'export FREQDIV='$freqdiv >> env.sh
echo 'export SPI_MODE='$spi_mode >> env.sh
echo 'export MODE='$mode >> env.sh
echo 'export SPI_SIZE_MAP='$spi_size_map >> env.sh
echo 'export FLASH_SIZE='$flash_size >> env.sh
# better define user_start_addr into Makefile
#echo 'export USER_START_ADDR='$user_start_addr >> env.sh
echo 'export LD_REF='$ld_ref >> env.sh
echo 'export FLASH_OPTIONS='\"$flash_options\" >> env.sh
echo 'export FLASH_INIT='\"$flash_init\" >> env.sh
