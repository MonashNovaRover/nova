/**
 * Author: Connor Macdougall
 * This component displays a spectrum for the dataset
 */

import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";

interface IDataChartProps {
    name: string,
    data: number[][],
    chartOptions: ApexOptions,
    peaks?: number[][],
}

const DatasetChart: React.FC<IDataChartProps> = (props: IDataChartProps) => {
    const dataChartOptions = ():ApexOptions => {
        if (props.peaks != undefined) {
            let peaklabels: PointAnnotations[] = props.peaks.map((value): PointAnnotations => {return {x: value[0], y: value[1], label: {text: value[0].toString() + ", " + value[1].toString()}}})
            return {...props.chartOptions, title: {...props.chartOptions.title, text: props.name},
                annotations: {
                    points: peaklabels
                }}
        } else {
            return {...props.chartOptions, title: {...props.chartOptions.title, text: props.name}}
        }
    }

    return (
        <div className="flex flex-col">
            <ReactApexChart options={dataChartOptions()} series={[{name: props.name,
            data: props.data}]} />
        </div>
    )
}

export default DatasetChart;