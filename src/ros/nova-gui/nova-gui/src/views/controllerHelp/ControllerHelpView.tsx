import React from 'react';
import controller from "../../assets/controller.png";
import controller_back from "../../assets/controller_back.png";

const ControllerHelpView: React.FC = () => {
    return (
        <div className="p-8 text-center">
            <h1 className="text-3xl font-bold mb-4">Xbox Controller Help</h1>
            <div className="flex items-center justify-center">
                <div className="mr-8">
                    <img className="rounded-md w-128 h-64" src={controller} alt="Xbox Controller" />
                    <img className='mt-4 rounded-md w-128 h-64' src={controller_back} alt='Xbox Controller Back' />

                </div>
                <div className="text-left">
                    <div className="flex">
                        <div className="mr-4">
                            <h2 className="text-xl font-semibold mb-2">Left Side</h2>
                            <ul>
                                <li><strong>1 -</strong> Rover Forward/Back/Left/Right</li>
                                <li><strong>2 -</strong> Strafe Mode</li>
                                <li><strong>3 -</strong> Lock GamePad</li>
                                <li><strong>4 -</strong> Xbox button</li>
                                <li><strong>5 -</strong> PlaceHolder</li>
                                <li><strong>8 -</strong> DPAD Y      |  Speed Incr/Decr Course</li>
                                <li><strong>8 -</strong> DPAD X      |  Speed Incr/Decr Fine</li>
                                <li><strong>9 -</strong> 3.5-mm port</li>
                                <li><strong>10 -</strong> Expansion port</li>
                               

                            </ul>
                        </div>
                        <div>
                            <h2 className="text-xl font-semibold mb-2">Right Side</h2>
                            <ul>
                                
                                <li><strong>6 -</strong> Unlock GamePad</li>
                                <li><strong>7 -</strong> Pivot Mode</li>
                                <li><strong>11 -</strong> Reduce Rover Speed by 40%</li>
                                <li><strong>12 -</strong> PLaceHolder</li>
                                <li><strong>13 -</strong> USB-C power port</li>
                                <li><strong>14 -</strong> Pair button</li>
                                <li><strong>Y -</strong> Tank Mode</li>
                                <li><strong>X -</strong> PlaceHolder</li>
                                <li><strong>B -</strong> Manual Control</li>
                                <li><strong>A -</strong> Autonomous Control</li>

                            </ul>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default ControllerHelpView;
