/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import {Card, CardFooter, CardProps} from "@nextui-org/react";
import React, {memo, useCallback, useEffect, useMemo} from "react";
import { RootState } from "../../../redux/RootState.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { ChartOptions, ChartStyle } from "../SpectraDisplay/ChartOptions.ts";
import DataChart from "../SpectraDisplay/DataChart.tsx";
import RamanLocalStorageSaveButton from "./RamanLocalStorageSaveButton.tsx";
import useDownload from "../../../hooks/useDownload.ts";

export interface RamanOutputProps extends CardProps {
    onSave?: (data: number[][], name: string) => void,
}


const RamanOutputUnmemoed: React.FC<RamanOutputProps> = (props) => {
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

    const download = useDownload("raman.csv", () => {
        const lines = ["wavelength,intensity"];
        for (let i = 0; i < determinedOutput[0].data.length; i++)
            lines.push(`${determinedOutput[0].data[i][0]},${determinedOutput[0].data[i][1]}`);

        return lines.join('\n');
    }, [determinedOutput], { type: "text/csv;charset=utf-8" })

    // A function called whenever the save button is pressed
    const onSave = useCallback((graphName: string) => {
        // ! I just take the first element of the array. I don't know if this is correct
        // TODO: Verify correctness
        const output = determinedOutput[0];
        props.onSave?.(output.data, graphName);
    }, [determinedOutput, props])

    return (
        <Card className={spectrumStore.isvalid ? "m-2 p-2" : "m-2 p-2 bg-rose-900"}>
            <DataChart dataset={determinedOutput} chartOptions={ChartOptions(ChartStyle.Default)} />
            <CardFooter>
                <RamanLocalStorageSaveButton onSave={onSave} onCSVSave={download}/>
            </CardFooter>
        </Card>
    )
}

// This might be overkill
const RamanOutput = memo(RamanOutputUnmemoed);
export default RamanOutput;