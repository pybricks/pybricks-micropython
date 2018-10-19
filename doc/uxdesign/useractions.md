#### Developer reference for expected outcome of user actions, given the current state of the hub


| Action \ State                                | Idle & BT     | Idle & no BT  | Program running & BT | Program running & No BT | Download in progress (Download button) | Download in progress (Download & Run button) |
|-----------------------------------------------|---------------|---------------|----------------------|-------------------------|----------------------------------------|----------------------------------------------|
| Single button click (release, press, release) | Start program | Start program | Stop program         | Stop program            |                                        |                                              |
| Long button press (beginning to press)        |               |               |                      |                         |                                        |                                              |
| Long button press (pressed for 5 seconds)     | Power off     | Power off     | Power off            | Power off               | Power off                              |  Power off                                   |
| IDE: BT Connect                               |               |               |                      |                         |                                        |                                              |
| IDE: BT Disconnect                            | Disconnect    | x             | Stop & disconnect    | x                       |                                        |                                              |
| IDE: Download                                 |               |               |                      |                         |                                        |                                              |
| IDE: Download & Run                           |               |               |                      |                         |                                        |                                              |

x: Don't allow: e.g. by graying out that button in the IDE
