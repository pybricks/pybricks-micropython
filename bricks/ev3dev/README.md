# Pybricks for EV3

## Building

See the [docker](./docker) folder for build instructions.

## Development tricks for Ubuntu

The following tips can help speed up development on Ubuntu.

Once connected, you can mount the robot file system locally for easy access:

```
gio mount ssh://robot@192.168.133.101/home/robot
```

Run the following on the brick once after preparing the microSD card.

```
rm -f pybricks-micropython
touch pybricks-micropython
chmod +x pybricks-micropython
sudo rm -f /usr/bin/pybricks-micropython
sudo ln -s /home/robot/pybricks-micropython /usr/bin/pybricks-micropython
```

Once you've followed the steps above, you can build and deploy using:

```
make ev3dev-armel && cp bricks/ev3dev/pybricks-micropython /run/user/1000/gvfs/sftp:host=*,user=robot/home/robot
```

Then all pybricks-micropython tools and projects will use the new build without
requiring further modifications in the project.

Start Pybricks and run commands to initialize it for easy testing:

```
brickrun --redirect -- pybricks-micropython -i -c "from core import *; print('add even more init here')"
```
