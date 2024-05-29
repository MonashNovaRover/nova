/**
 * Author: Connor Macdougall
 * This component acts as a container for the three distinct components.
 * Use for simple layout configuration changes, and exporting.
 */

import React, { useState } from "react";
import { Button, Input, Card } from "@nextui-org/react";
import RamanCCDInputs from "./RamanCCDInputs";
import RamanMechanicalInputs from "./RamanMechanicalInputs";
import RamanOutput, {RamanOutputProps} from "./RamanOutput";

const RamanSpec: React.FC<RamanOutputProps> = (props) => {
    const sampleFilename = "graph_site1_sample1";
    const example_label = "Example: "
    const [file, setFile] = useState(sampleFilename);

    return (
        <div className="flex flex-row m-3 rounded-xl bg-zinc-800">
            <div className="w-1/2 flex flex-col space-y-10 p-2 py-8">
                <RamanMechanicalInputs />
                <RamanCCDInputs />
                <Card className="flex flex-row justify-around py-4">
                    <Input 
                        onValueChange={(value: string) => setFile(value)} 
                        className="shrink-0 w-60" 
                        type="sample" label="Sample Filename" 
                        placeholder={example_label.concat(sampleFilename)} 
                        defaultValue={file}
                        endContent={
                            <div className="pointer-events-none flex items-center">
                            <span className="text-default-400 text-small">.csv</span>
                            </div>
                        } 
                    />
                    <Button 
                        onPress={() => {}}
                        color= "primary" className="h-14 shrink-0 w-1/2" radius="lg">
                        Save
                    </Button>
                </Card>
            </div>
            <div className="w-1/2">
                <RamanOutput onSave={props.onSave}/>
            </div>
        </div>
    )
}

export default RamanSpec;