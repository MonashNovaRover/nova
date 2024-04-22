/**
 * This component displays a spectrum within the dataset
 * Author: Connor Macdougall
 */

import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";

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
    // adds peak labels to styling if peaks are given
    // always adds 'name' string as title
    let dataChartOptions: ApexOptions = {...props.chartOptions, title: {...props.chartOptions.title, text: props.dataset[0].name}}
    if (props.peaks != undefined) {
        let peaklabels: PointAnnotations[] = props.peaks.map((value): PointAnnotations => {
            return {
                x: value[0], 
                y: value[1], 
                label: {text: value[0].toString() + ", " + value[1].toString()}
            }
        })
        dataChartOptions = {...props.chartOptions, title: {...props.chartOptions.title, text: props.dataset[0].name},
            annotations: {
                points: peaklabels
            }}
    }

    return (
        <ReactApexChart options={dataChartOptions} series={[{name: props.dataset[0].name, data: props.dataset[0].data}]} />
    )
}

export default DataChart;