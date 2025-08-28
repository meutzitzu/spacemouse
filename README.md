# DIY spacemouse

### short Video 
[![spacemouse thumbnail](https://img.youtube.com/vi/N7X5VpKxuyY/0.jpg)](https://www.youtube.com/watch?v=N7X5VpKxuyY)

## Description
This USB input device device has a handle which can be moved in all 6 directions (3 translation + 3 rotation). These motions are sent to compatible 3D applications for more intuitive navigation.

Although this is not the first open-source/open-hardware spacemouse project, it is one of the more
compact ones and it’s 3D-printed components are more carefully designed, first for reducing print
time to a minimum and also by using captive nuts instead of self-tapping screws directly into the
plastic or superglue. This makes it possible to assemble and disassemble a lot more times without
wearing out the holes and needing a re-print, making it more suitable as a starting project upon
which many more modifications can be designed.
Another unique feature of this project is the tilt of the handle towards the user. This makes the hand
movements a slightly more comfortable (in my opinion, as this is ultimately a matter of preference).

### building
Assuming you have
1. Platformio installed
2. your devboard connected via USB
3. your user added to `dialout` or `uucp` groups

building is as simple as 
```bash
./build.sh
```

But you should consider using the software from [here](https://github.com/AndunHH/spacemouse) as it has more features 
