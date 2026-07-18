/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import { useEffect } from "react";
import { RootState } from "../../../redux/RootState.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { ChartStyle } from "../SpectraDisplay/ChartOptions.ts";
import OutputComparison from "../SpectraDisplay/OutputComparison.tsx";
import { getDefaultPeakFinder } from "../SpectraDisplay/ChartAnalysis.ts";

const RamanOutputComparison: React.FC = () => {
    // units are nm
    //const LASER_GREEN_START = 416
    //const LASER_GREEN_END = 642
    //const LASER_GREEN_RANGE = LASER_GREEN_END - LASER_GREEN_START
    const LASER_RED_START = 700
    const LASER_RED_END = 928
    const LASER_RED_RANGE = LASER_RED_END - LASER_RED_START

    const RAMAN_PEAK_VALUE = 3600  // approx
    const NORMALISED_SCALE_MAX = 100

    // Bifrost
    const spectrumStore = useSelector(
        (state: RootState) => state.ramanSpecMessageStore
    );
    const bifrost = useBifrost({ topic: RosTopic.RAMAN_SPEC_MSG });
    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    /*const greenLaserOutput = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [Math.round(100*(LASER_GREEN_START + LASER_GREEN_RANGE*index/spectrumStore.spectrum.length)) / 100.0, Math.round(100*NORMALISED_SCALE_MAX*element/RAMAN_PEAK_VALUE)/100.0])
    }]*/

    const redLaserOutput = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [Math.round(100*(LASER_RED_START + LASER_RED_RANGE*index/spectrumStore.spectrum.length)) / 100.0, Math.round(100*NORMALISED_SCALE_MAX*element/RAMAN_PEAK_VALUE)/100.0])
    }]

    const kerogendata = [10, 11, 9, 8, 9, 10, 12, 11, 9, 11, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 47, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]
    const kerogen2data = [10, 11, 9, 8, 9, 10, 12, 13, 12, 9, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 47, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]
    const elementData = [[{
        name: "Kerogen",
        data: kerogendata.map((element, index) => [50*index, element])
    }],[{
        name: "Kerogen 2",
        data: kerogen2data.map((element, index) => [50*index, element])
    }]]

    return (
        <OutputComparison
            title="Comparison and Analysis"
            peaksOnMain
            outputData={redLaserOutput}
            elementData={elementData}
            style={ChartStyle.Default}
            peakFinder={getDefaultPeakFinder(2, 20)}
        />
    ) 
}

export default RamanOutputComparison;