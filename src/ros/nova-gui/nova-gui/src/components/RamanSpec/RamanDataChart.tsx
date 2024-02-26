/**
 * Author: Connor Macdougall
 * This component displays an element's Raman spectrum
 */

import ReactApexChart from "react-apexcharts";
import { ApexOptions } from "apexcharts";
import { useState } from "react";
import { Button } from "@nextui-org/react";

export interface IRamanDataChartProps {
    name: string,
    data: number[][],
    addToMainOverlay: (name: string, data: number[][])=>void,
    removeFromMainOverlay: (name: string)=>void
}

const RamanDataChart: React.FC<IRamanDataChartProps> = (props: IRamanDataChartProps) => {
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
        colors: ["#992F7B", "#C4841D"],
        annotations: {
            points: [{
                x: 1350,
                y: 70,
                label: {
                    text: '1350'
                }
            }, {
                x: 1600,
                y: 90,
                label: {
                    text: '1600'
                }
            }]
        }
      };

    const [onMain, setOnMain] = useState(false);

    return (
        <div className="flex flex-col">
            <ReactApexChart options={outputChartOptions} series={[{name: props.name,
            data: props.data}]} />
            <Button 
            onPress={() => {
                if (onMain) {
                    props.removeFromMainOverlay(props.name)
                } else {
                    props.addToMainOverlay(props.name, props.data)
                }
                setOnMain(!onMain);
            }}
            color= {onMain ? "secondary" : "primary"} radius="lg">
                {props.name}{onMain ? "Remove" : ""}
            </Button>
        </div>
    )
}

export default RamanDataChart;