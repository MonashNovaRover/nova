import React from 'react';
import controller from "../../assets/controller.png";
import controller_back from "../../assets/controller_back.png";

const ControllerHelpView: React.FC = () => {
    return (
        <div className="p-4 text-center text-black">
            <h1 className="text-2xl font-bold mb-2">Xbox Controller Help</h1>
            <div className="flex items-center justify-center">
                <div className="mr-4">
                    <img className="rounded-md w-64 h-32" src={controller} alt="Xbox Controller" />
                    <img className='mt-2 rounded-md w-64 h-32' src={controller_back} alt='Xbox Controller Back' />
                </div>
                <div className="text-left">
                    <div className="flex">
                        <div className="mr-2">
                            <ul>
                                <li><strong>1 -</strong> Rover Forward/Back/Left/Right</li>
                                <li><strong>2 -</strong> Strafe Mode</li>
                                <li><strong>3 -</strong> Lock GamePad</li>
                                <li><strong>4 -</strong> Xbox button</li>
                                <li><strong>5 -</strong> PlaceHolder</li>
                                <li><strong>8 -</strong> DPAD Y | Speed Incr/Decr Course</li>
                                <li><strong>8 -</strong> DPAD X | Speed Incr/Decr Fine</li>
                            </ul>
                        </div>
                        <div>
                            <ul>
                                <li><strong>6 -</strong> Unlock GamePad</li>
                                <li><strong>7 -</strong> Pivot Mode</li>
                                <li><strong>11 -</strong> Reduce Rover Speed by 40%</li>
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
