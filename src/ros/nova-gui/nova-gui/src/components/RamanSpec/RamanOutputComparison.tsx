/**
 * Author: Connor Macdougall
 * This component outputs responses from the CCD, and also has graphs to compare output data against.
 * It accepts responses from the 'raman_spectra' ROS service.
 * TODO:
 *  - Overhaul how graphs are overlayed to a manner that can fit single collection mode (continuous mode could just remove all overlayed graphs)
 */

import { Card, CardHeader, ScrollShadow } from "@nextui-org/react";
import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";
import RamanDataChart from "./RamanDataChart";
import { useState, useEffect } from "react";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";

const RamanOutputComparison: React.FC = () => {
    // Bifrost
    const spectrumStore = useSelector(
        (state: RootState) => state.ramanSpecMessageStore
    );
    
    const bifrost = useBifrost({ topic: RosTopic.RAMAN_SPEC_MSG });
    
    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const [outputChartSeries, setOutputChartSeries] = useState([{
        name: "CCD Output",
        data: [1,2]
    }]);

    let fakeOutputChartSeries: ApexAxisChartSeries = [{
        name: "CCD Output",
        data: spectrumStore.spectrum.map((element, index) => [50*index, element])
    }];

    const addToMainOverlay = (Cname: string, Cdata: number[][]) => {
        let result = outputChartSeries;
        setOutputChartSeries(result.concat([{
            name: Cname,
            data: Cdata[0]
        }]));
    }
    const removeFromMainOverlay = (Cname: string) => {
        let index: number = -1;
        for (let i = 0; i < outputChartSeries.length; i++) {
            if (outputChartSeries[i].name == Cname) {
                index = i;
            }
        }
        if (index >= 0) {
            let result = outputChartSeries;
            result.splice(index, 1);
            setOutputChartSeries([...result]);
        }
    }

    const kerogendata = [10, 11, 9, 8, 9, 10, 12, 11, 9, 11, 10, 11, 10, 9, 11, 12, 13, 15, 17, 20, 23, 24, 28, 33, 39, 47, 58, 70, 66, 54, 50, 70, 90, 65, 40, 35, 34, 34, 35, 35, 34, 34, 33, 32, 31, 31, 30, 31, 32]

    const elementData = [[{
        name: "Kerogen",
        data: kerogendata.map((element, index) => [50*index, element])
    }]]

    const outputChartOptions: ApexOptions = {
        title: {
            text: 'CCD Output',
            align: "center",
            floating: true,
            style: {
                fontSize: '18px',
                color: '#fff'
            }
        },
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
            min: 0,
            max: 2400,
            title: {
                text: 'Raman Shift (1/cm)',
                style: {
                    fontSize: '14px',
                    color: '#fff'
                }
            },
            labels: {
                show: true,
            }
        },
        yaxis: {
            min: 0,
            max: 100,
            title: {
                text: 'Normalised intensity',
                style: {
                    fontSize: '14px',
                    color: '#fff'
                }
            },
            labels: {
                show: false,
            }
        },
        grid: {
            show: false
        },
        colors: ["#992F7B", "#C4841D", "#66AAF9", "#F31260"]
    };

    let ccdOutputChart;
    if (spectrumStore.spectrum.length == 0) {
        ccdOutputChart = <Card className="text-center w-1/2 self-center p-2 m-1">Use the CCD Inputs to request Output</Card>;
    } else if (spectrumStore.isvalid) {
        ccdOutputChart = <ReactApexChart className = "w-1/2 self-center" options={outputChartOptions} series={fakeOutputChartSeries} />;
    } else {
        ccdOutputChart = <Card className="text-center w-1/2 self-center p-2 m-1">Uh Oh</Card>;
    }

    return (
        <Card className="w-fit p-2 m-1 w-auto">
            <CardHeader className="shrink-0 w-48 p-1">Comparison and Analysis</CardHeader>
            <div className="flex flex-row">
                {ccdOutputChart}
                <ScrollShadow hideScrollBar className="w-1/2 h-154">
                    {elementData.map( element => (<RamanDataChart key={element[0].name} addToMainOverlay={addToMainOverlay} removeFromMainOverlay={removeFromMainOverlay} name={element[0].name} data={element[0].data} />))}
                </ScrollShadow>
            </div>
        </Card>
    ) 
}

export default RamanOutputComparison;