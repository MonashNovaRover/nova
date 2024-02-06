/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 */

import { Card, CardHeader, ScrollShadow } from "@nextui-org/react";
import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";
import RamanDataChart from "./RamanDataChart";
import { useState } from "react";

const RamanOutputComparison: React.FC = () => {
    const [outputChartSeries, setOutputChartSeries] = useState([{
        name: "CCD Output",
        data: [5, 15, 30, 40, 23, 17, 10, 62, 52, 73, 49, 21]
    }]);

    const addToMainOverlay = (Cname: string, Cdata: number[]) => {
        let result = outputChartSeries;
        setOutputChartSeries(result.concat([{
            name: Cname,
            data: Cdata
        }]));
    }
    const removeFromMainOverlay = (Cname: string) => {
        let index: number = -1;
        for (let i = 0; i < outputChartSeries.length; i++) {
            if (outputChartSeries[i].name == Cname) {
                index = i;
                console.log(i);
            }
        }
        if (index >= 0) {
            let result = outputChartSeries;
            result.splice(index, 1);
            setOutputChartSeries([...result]);
        }
    }

    const elementData = [[{
        name: "Element 1",
        data: [1, 3, 6, 65, 43, 32, 35, 24, 18, 15, 16, 8]
    }], [{
        name: "Element 2",
        data: [1, 3, 6, 25, 43, 62, 71, 35, 18, 15, 16, 8]
    }], [{
        name: "Element 3",
        data: [1, 3, 6, 13, 23, 32, 35, 24, 65, 24, 16, 8]
    }]]

    const outputChartOptions: ApexOptions = {
        stroke: {
            curve: "smooth"
        },
        chart: {
            animations: {enabled: false},
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
        colors: ["#992F7B", "#C4841D", "#66AAF9", "#F31260"]
    };

    return (
        <Card className="w-fit p-2 m-1 w-auto">
            <CardHeader className="shrink-0 w-48 p-1">Comparison and Analysis</CardHeader>
            <div className="flex flex-row">
                <ReactApexChart className = "w-1/2" options={outputChartOptions} series={outputChartSeries} />
                <ScrollShadow hideScrollBar className="w-1/2 h-154">
                    {elementData.map( element => (<RamanDataChart key={element[0].name} addToMainOverlay={addToMainOverlay} removeFromMainOverlay={removeFromMainOverlay} name={element[0].name} data={element[0].data} />))}
                </ScrollShadow>
            </div>
        </Card>
    ) 
}

export default RamanOutputComparison;