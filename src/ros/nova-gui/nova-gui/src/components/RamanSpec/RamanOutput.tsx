/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import { Card } from "@nextui-org/react";
import { useEffect } from "react";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";
import { ChartOptions, ChartStyle } from "../SpectraDisplay/ChartOptions";
import DataChart from "../SpectraDisplay/DataChart";
import { getDefaultPeakFinder } from "../SpectraDisplay/ChartAnalysis";

const RamanOutput: React.FC = () => {
    // units are nm
    const LASER_GREEN_START = 416
    const LASER_GREEN_END = 642
    const LASER_GREEN_RANGE = LASER_GREEN_END - LASER_GREEN_START
    const LASER_RED_START = 700
    const LASER_RED_END = 928
    const LASER_RED_RANGE = LASER_RED_END - LASER_RED_START

    const RAMAN_PEAK_VALUE = 3600  // approx
    const NORMALISED_SCALE_MAX = 100

    const ramanMechState = useSelector(
        (state: RootState) => state.ramanMechMessageStore
    );

    // Bifrost
    const spectrumStore = useSelector(
        (state: RootState) => state.ramanSpecMessageStore
    );
    const bifrost = useBifrost({ topic: RosTopic.RAMAN_SPEC_MSG });
    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    let greenLaserOutput = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [Math.round(100*(LASER_GREEN_START + LASER_GREEN_RANGE*index/spectrumStore.spectrum.length)) / 100.0, Math.round(100*NORMALISED_SCALE_MAX*element/RAMAN_PEAK_VALUE)/100.0])
    }]

    let redLaserOutput = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [Math.round(100*(LASER_RED_START + LASER_RED_RANGE*index/spectrumStore.spectrum.length)) / 100.0, Math.round(100*NORMALISED_SCALE_MAX*element/RAMAN_PEAK_VALUE)/100.0])
    }]

    const determineOutput = () => {
        if (ramanMechState.green_laser_on) {
            return greenLaserOutput
        } else {
            return redLaserOutput
        }
    }
    
    const peakFinder = getDefaultPeakFinder(3, 20);

    return (
        <Card className="m-2 p-2">
            <DataChart dataset={determineOutput()} chartOptions={ChartOptions(ChartStyle.Default)} peaks={peakFinder(determineOutput()[0].data)} />
        </Card>
    ) 
}

export default RamanOutput;