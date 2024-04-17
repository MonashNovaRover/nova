/*
 * Presets for chart styling
 * Author: Connor Macdougall
*/

import { ApexOptions } from "apexcharts";

export enum ChartStyle {
    Default,
    RamanMain,
    RamanData,
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
          };
    }
    else if (style == ChartStyle.RamanMain) {
        return {
            title: {
                text: 'CCD Output',
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
    } else if (style == ChartStyle.RamanData) {
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
          };
    } else {
        return ChartOptions(ChartStyle.Default);
    }
}