/*
 * Presets for chart styling
 * Author: Connor Macdougall
 * Modified by: Binuda Kalugalage
*/

import { ApexOptions } from "apexcharts";

export enum ChartStyle {
    Default,
    Signal,
    Received,
    Sent,
    Ping
}

export const ChartOptions = (style: ChartStyle, numPoints: number): ApexOptions => {
    if (style == ChartStyle.Signal) {
        return {
            title: {
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
                animations: { enabled: false },
                type: 'bar',
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
                    text: `Last ${numPoints} seconds`,
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
                min: undefined,
                max: undefined,
                forceNiceScale: true,
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
    }
    else if (style == ChartStyle.Received) {
        return {
            title: {
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
                animations: { enabled: false },
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
                    text: `Last ${numPoints} seconds`,
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
                min: undefined,
                max: undefined,
                forceNiceScale: true,
                title: {
                    text: 'Received (kbps)',
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
    }
    else if (style == ChartStyle.Sent) {
        return {
            title: {
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
                animations: { enabled: false },
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
                    text: `Last ${numPoints} seconds`,
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
                min: undefined,
                max: undefined,
                forceNiceScale: true,
                title: {
                    text: 'Sent (kbps)',
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
    }
    else if (style == ChartStyle.Ping) {
        return {
            title: {
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
                animations: { enabled: false },
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
                    text: `Last ${numPoints} seconds`,
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
                min: undefined,
                max: undefined,
                forceNiceScale: true,
                title: {
                    text: 'Ping (ms)',
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
    }
    else {
        return ChartOptions(ChartStyle.Default, 30);
    }
}