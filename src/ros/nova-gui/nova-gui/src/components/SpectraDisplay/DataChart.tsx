/**
 * This component displays a spectrum within the dataset
 * Author: Connor Macdougall
 */

import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";
import React, {useMemo} from "react";

export type ApexDataset = {
    name: string,       // ApexAxisChartSeries has this optional whereas it is expected in this implementation (as it is unique to each dataset)
    data: number[][]    // (x,y) coordinates
}[]

interface IDataChartProps {
    dataset: ApexDataset
    chartOptions: ApexOptions,  // chart styling
    peaks?: number[][],         // (x,y) coordinates of all peaks
}

const DataChart: React.FC<IDataChartProps> = (props: IDataChartProps) => {
    // I wrapped your dataChartOptions calculation in a function, that returns the value for dataChartOptions.
    // This will cache the calculation, until some input of the calculation changes
    const dataChartOptions = useMemo<ApexOptions>(() => {
        // adds peak labels to styling if peaks are given
        // always adds 'name' string as title
        if (props.peaks === undefined)
            return {...props.chartOptions, title: {...props.chartOptions.title, text: props.dataset[0].name}}

        const peakLabels: PointAnnotations[] = props.peaks.map((value): PointAnnotations => {
            return {
                x: value[0],
                y: value[1],
                label: {text: value[0].toString() + ", " + value[1].toString()}
            }
        });

        return {
            ...props.chartOptions,
            title: {
                ...props.chartOptions.title,
                text: props.dataset[0].name
            },
            annotations: {
                points: peakLabels
            }
        };
    }, [props.chartOptions, props.dataset, props.peaks]);

    // I yoinked this out of the JSX, so it could be cached
    const series = useMemo<ApexAxisChartSeries>(() => (
        //[{name: props.dataset[0].name, data: props.dataset[0].data}]
        // TODO: Verify that I'm not missing something by replacing the above line with below:
        [props.dataset[0]]
    ), [props.dataset])

    return (
        <ReactApexChart
          key={props.dataset[0].name}
          options={dataChartOptions}
          series={series}
        />
    )
}

export default DataChart;