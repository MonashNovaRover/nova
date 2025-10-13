/*
 * Presets for chart styling
 * Author: Connor Macdougall
*/

import { ApexOptions } from "apexcharts";

export enum ChartStyle {
    Default,
}

export const ChartOptions = (style: ChartStyle): ApexOptions => {
    if (style == ChartStyle.Default) {
        return {
            title: {
                text: '',
                align: "center",
                floating: true,
                style: {
                    fontSize: '18px',
                    color: '#fff'
                },
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
                title: {
                    text: 'Time (s)',
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
                min:-100,
                max: 50,
                title: {
                    text: 'Decibels (dB)',
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
          };
    } else {
        return ChartOptions(ChartStyle.Default);
    }
}