/**
 * Author: Connor Macdougall
 * This component acts as a container for the three distinct components.
 * Use for simple layout configuration changes, and exporting.
 */


import { Card, CardBody, CardHeader } from "@nextui-org/react";
import RamanCCDInputs from "./RamanCCDInputs.tsx";
import RamanMechanicalInputs from "./RamanMechanicalInputs.tsx";
import RamanOutput, {RamanOutputProps} from "./RamanOutput.tsx";

const RamanSpec: React.FC<RamanOutputProps> = (props) => {
    return (
        <Card {...props}>
            <CardHeader>Raman Spec</CardHeader>
            <CardBody>
                <div className="flex flex-row m-3 rounded-xl bg-zinc-800">
                    <div className="w-1/2 flex flex-col space-y-10 p-2 py-8">
                        <RamanMechanicalInputs />
                        <RamanCCDInputs />
                    </div>
                    <div className="w-1/2">
                        <RamanOutput onSave={props.onSave}/>
                    </div>
                </div>
            </CardBody>
        </Card>
    )
}

export default RamanSpec;