/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import {Button, Card, CardFooter, Input} from "@nextui-org/react";
import React, {useCallback, useEffect, useMemo, useState} from "react";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";
import { ChartOptions, ChartStyle } from "../SpectraDisplay/ChartOptions";
import DataChart from "../SpectraDisplay/DataChart";

export interface RamanOutputProps {
    onSave?: (data: number[][], name: string) => void,
}


const RamanOutput: React.FC<RamanOutputProps> = (props) => {

    const [graphName, setGraphName] = useState<string>("CCD Output")

    // units are nm
    const LASER_GREEN_START = 416
    const LASER_GREEN_END = 642
    const LASER_GREEN_RANGE = LASER_GREEN_END - LASER_GREEN_START
    const LASER_RED_START = 700
    const LASER_RED_END = 928
    const LASER_RED_RANGE = LASER_RED_END - LASER_RED_START
    
    const NORMALISED_SCALE_MAX = 1
    const DECIMAL_PLACE_ROUNDING_FROM_MAX = 4
    const POINTS_ON_GRAPH = 200

    const ramanMechState = useSelector(
        (state: RootState) => state.ramanMechMessageStore
    );

    // Bifrost
    const spectrumStore = useSelector(
        (state: RootState) => state.ramanSpecMessageStore
    );
    const STEP_VALUE = Math.floor(spectrumStore.spectrum.length / POINTS_ON_GRAPH)

    const bifrost = useBifrost({ topic: RosTopic.RAMAN_SPEC_MSG });
    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const maxOutputValue = useMemo(() => (
      Math.max(...spectrumStore.spectrum)
    ), [spectrumStore.spectrum])
    const greenLaserOutput = useMemo(() => (
      spectrumStore.spectrum.map((element, index) => [Math.round((10**DECIMAL_PLACE_ROUNDING_FROM_MAX)*(LASER_GREEN_START + LASER_GREEN_RANGE*index/spectrumStore.spectrum.length)) / ((10**DECIMAL_PLACE_ROUNDING_FROM_MAX)* 1.0), Math.round((10**DECIMAL_PLACE_ROUNDING_FROM_MAX)*NORMALISED_SCALE_MAX*(1 - (element/maxOutputValue)))/((10**DECIMAL_PLACE_ROUNDING_FROM_MAX) * 1.0)])
    ), [LASER_GREEN_RANGE, maxOutputValue, spectrumStore.spectrum])
    const redLaserOutput = useMemo(() => (
      spectrumStore.spectrum.map((element, index) => [Math.round((10**DECIMAL_PLACE_ROUNDING_FROM_MAX)*(LASER_RED_START + LASER_RED_RANGE*index/spectrumStore.spectrum.length)) / ((10**DECIMAL_PLACE_ROUNDING_FROM_MAX) * 1.0), Math.round((10**DECIMAL_PLACE_ROUNDING_FROM_MAX)*NORMALISED_SCALE_MAX*(1 - (element/maxOutputValue)))/((10**DECIMAL_PLACE_ROUNDING_FROM_MAX) * 1.0)])
    ), [LASER_RED_RANGE, maxOutputValue, spectrumStore.spectrum])

    const determinedOutput = useMemo(() => {
        let output = redLaserOutput;
        if (ramanMechState.green_laser_on) {
            output = greenLaserOutput;
        }
        output = output.filter((_, index) => index % STEP_VALUE == 0)
        return [{
            name: "CCD Output",
            data: output
        }]
    }, [STEP_VALUE, greenLaserOutput, ramanMechState.green_laser_on, redLaserOutput]);

    // A function called whenever the save button is pressed
    const onSave = useCallback(() => {
        // ! I just take the first element of the array. I don't know if this is correct
        // TODO: Verify correctness
        const output = determinedOutput[0];
        props.onSave?.(output.data, graphName);
    }, [determinedOutput, graphName, props])

    return (
        <Card className={spectrumStore.isvalid ? "m-2 p-2" : "m-2 p-2 bg-rose-900"}>
            <DataChart dataset={determinedOutput} chartOptions={ChartOptions(ChartStyle.Default)} />
            <CardFooter>
                <div className="flex flex-row gap-3 my-3 mb-0">
                    <Input size="sm" placeholder="Graph name" onValueChange={setGraphName} value={graphName}></Input>
                    <Button
                        color={graphName.length > 0 ? "success" : "default"}
                        isDisabled={graphName.length === 0}
                        onPress={onSave}
                        size="sm"
                    >
                        Save
                    </Button>
                </div>
            </CardFooter>
        </Card>
    )
}

export default RamanOutput;