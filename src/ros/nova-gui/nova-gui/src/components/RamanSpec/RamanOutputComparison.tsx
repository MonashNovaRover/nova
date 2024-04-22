/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import { useEffect } from "react";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";
import { ChartStyle } from "../SpectraDisplay/ChartOptions";
import OutputComparison from "../SpectraDisplay/OutputComparison";
import { defaultPeakFinder } from "../SpectraDisplay/ChartAnalysis";

const RamanOutputComparison: React.FC = () => {
    // Bifrost
    const spectrumStore = useSelector(
        (state: RootState) => state.ramanSpecMessageStore
    );
    const bifrost = useBifrost({ topic: RosTopic.RAMAN_SPEC_MSG });
    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    let outputChartSeries = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [50*index, element])
    }]

    const kerogendata = [10, 11, 9, 8, 9, 10, 12, 11, 9, 11, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 47, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]
    const elementData = [[{
        name: "Kerogen",
        data: kerogendata.map((element, index) => [50*index, element])
    }]]

    return (
        <OutputComparison
            title="Comparison and Analysis" 
            outputData={outputChartSeries}
            elementData={elementData}
            outputStyle={ChartStyle.DefaultMain}
            datasetStyle={ChartStyle.DefaultData}
            peakFinder={defaultPeakFinder}
        />
    ) 
}

export default RamanOutputComparison;