# Sends data for the spectrometer sensor (fake data)

# Gets the chosen cuvette number from the first argument
if [ -z $1 ]
then
    cuvette=01
else
    cuvette=0$1
    if [ $1 -eq 10 ]; then
        cuvette=0A;
    elif [ $1 -eq 11 ]; then
        cuvette=0B;
    elif [ $1 -eq 12 ]; then
        cuvette=0C;
    elif [ $1 -eq 13 ]; then
        cuvette=0D;
    elif [ $1 -eq 14 ]; then
        cuvette=0E;
    elif [ $1 -eq 15 ]; then
        cuvette=0F;
    elif [ $1 -eq 16 ]; then
        cuvette=10;
    elif [ $1 -eq 17 ]; then
        cuvette=11;
    elif [ $1 -eq 18 ]; then
        cuvette=12;
    elif [ $1 -eq 19 ]; then
        cuvette=13;
    elif [ $1 -eq 20 ]; then
        cuvette=14;
    fi
fi

# Create a function for a random hex value
function random_hex () {
    rand=$[ $RANDOM % 16 ]
    if [ $rand -eq 10 ]; then
        val=A;
    elif [ $rand -eq 11 ]; then
        val=B;
    elif [ $rand -eq 12 ]; then
        val=C;
    elif [ $rand -eq 13 ]; then
        val=D;
    elif [ $rand -eq 14 ]; then
        val=E;
    elif [ $rand -eq 15 ]; then
        val=F;
    else
        val=$rand
    fi
}

# Create a function for random hex value
function random_byte () {
    random_hex
    _a=$val
    random_hex
    _b=$val
    val="$_a$_b"
}

function random_int () {
    random_byte
    _c=$val
    random_byte
    _d=$val
    val="$_c.$_d"
}

function random_line () {
    random_int
    _e=$val
    random_int
    _f=$val
    random_int
    _g=$val
    val="$_e.$_f.$_g"
}

# Sends 5 frames of FDA
cansend can1 007#00.01.$cuvette.00.00.00.00.00

random_line
cansend can1 007#01.01.$val
random_line
cansend can1 007#02.01.$val
random_line
cansend can1 007#03.01.$val
random_line
cansend can1 007#04.01.$val

# Sends 3 frames of BCA
cansend can1 008#00.01.$cuvette.00.00.00.00.00

random_line
cansend can1 008#01.01.$val
random_line
cansend can1 008#02.01.$val