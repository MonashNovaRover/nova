/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import {Card, CardHeader, ScrollShadow} from "@nextui-org/react";
import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";

const RamanOutputComparison: React.FC = () => {
    let outputChartSeries: ApexAxisChartSeries = [{
        name: "CCD Output",
        data: [5, 15, 30, 40, 23, 17, 10, 62, 52, 73, 49, 21]
    }]
    let datasetChart1: ApexAxisChartSeries = [{
        name: "Element 1",
        data: [1, 3, 6, 65, 43, 32, 35, 24, 18, 15, 16, 8]
    }]
    let datasetChart2: ApexAxisChartSeries = [{
        name: "Element 2",
        data: [1, 3, 6, 25, 43, 62, 71, 35, 18, 15, 16, 8]
    }]
    let datasetChart3: ApexAxisChartSeries = [{
        name: "Element 3",
        data: [1, 3, 6, 13, 23, 32, 35, 24, 65, 24, 16, 8]
    }]
    let outputChartOptions: ApexOptions = {
        stroke: {
            curve: "smooth"
        },
        chart: {
          type: 'line',
          background: "000",
          toolbar: {
            show: false
          }
        },
        tooltip: {
            theme: "dark",
            fixed: {
                offsetX: 10,
                offsetY: 10
            }
        },
        xaxis: {
            labels: {
                show: false
            }
        },
        grid: {
            show: false
        },
        colors: ["#992F7B", "#C4841D"]
      };

    return (
        <Card className="w-fit p-2 m-1 w-auto">
            <CardHeader className="shrink-0 w-48 p-1">Comparison and Analysis</CardHeader>
            <div className="flex flex-row">
                <ReactApexChart className = "w-1/2" type="line" options={outputChartOptions} series={outputChartSeries} />
                <ScrollShadow hideScrollBar className="w-1/2 h-142">
                    <ReactApexChart className="w-1/2" type="line" options={outputChartOptions} series={datasetChart1} />
                    <ReactApexChart className="w-1/2" type="line" options={outputChartOptions} series={datasetChart2} />
                    <ReactApexChart className="w-1/2" type="line" options={outputChartOptions} series={datasetChart3} />
                </ScrollShadow>
            </div>
        </Card>
    ) 
}

export default RamanOutputComparison;