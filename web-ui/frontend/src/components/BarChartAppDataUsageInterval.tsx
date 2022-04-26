import { useEffect, useState, Fragment } from 'react';
import { AppSession } from '../interfaces';
import { convertToUnit, UNITS } from '../units';

import Chart from 'chart.js/auto';
import { ChartDataset } from 'chart.js';
import zoomPlugin from 'chartjs-plugin-zoom';
import 'chartjs-adapter-moment';

import { Menu, Transition } from '@headlessui/react';
import { ChevronDownIcon } from '@heroicons/react/solid';

const MS_MINUTE = 60000;
const MS_HOUR = 60 * MS_MINUTE;
const INTERVALS: string[] = [
  '10 MIN',
  '30 MIN',
  '1 HOUR',
  '12 HOURS',
  '24 HOURS',
];

interface BarChartAppDataUsagePerIntervalProps {
  appSessions: AppSession[];
  start: Date;
  end: Date;
  isDarkMode: boolean;
}
let barChart: Chart;

function addAlpha(color: string, opacity: number): string {
  const _opacity = Math.round(Math.min(Math.max(opacity || 1, 0), 1) * 255);
  return color + _opacity.toString(16).toUpperCase();
}

function BarChartAppDataUsagePerInterval(
  props: BarChartAppDataUsagePerIntervalProps
) {
  const [data, setData] = useState({
    labels: [] as Date[],
    datasets: [] as ChartDataset[],
  });
  const [yMax, setYMax] = useState(10);
  const [interval, setInterval] = useState(INTERVALS[0]);
  const [unitIndex, setUnitIndex] = useState(2);

  useEffect(() => {
    // Populate all possible label values as Unix time for faster comparison
    const tmpLabels: number[] = [];

    const msInterval =
      parseInt(interval.substring(0, interval.indexOf(' '))) *
      (interval.includes('MIN') ? MS_MINUTE : MS_HOUR);
    props.appSessions.forEach((appSess) => {
      appSess.sessions.forEach((sess) => {
        if (tmpLabels.length === 0) tmpLabels.push(sess.start);
        if (sess.start - tmpLabels[tmpLabels.length - 1] >= msInterval) {
          tmpLabels.push(sess.start);
        }
      });
    });
    tmpLabels.sort((a, b) => a - b);

    const tmpData = { labels: [] as Date[], datasets: [] as ChartDataset[] };
    tmpLabels.forEach((l) => tmpData.labels.push(new Date(l * 1000))); // Push labels into tmpData as Date type

    let max = 0;
    props.appSessions.forEach((appSess) => {
      const color: string = appSess.application.colorHex;
      const dataset = {
        label: appSess.application.name,
        data: [] as number[],
        borderColor: color,
        backgroundColor: addAlpha(color, 0.5),
      };

      const data: number[] = [];
      tmpLabels.forEach((label, i) => {
        appSess.sessions.forEach((s) => {
          if (Math.abs(label - s.start) > msInterval) return;

          let sum = convertToUnit(s.bytesTx + s.bytesRx, unitIndex);
          if (data[i]) {
            data[i] += sum;
            sum = data[i] + sum;
          } else data.push(sum);

          if (sum > max) max = sum;
        });
      });
      dataset.data = data;
      tmpData.datasets.push(dataset);
    });

    setYMax(Math.round(max * 1.1));
    setData(tmpData);
  }, [props.appSessions, props.isDarkMode, unitIndex]);

  useEffect(() => {
    buildChart();
  }, [data, unitIndex]);

  const buildChart = () => {
    Chart.register(zoomPlugin); // Must register before creating new Chart

    const options = {
      responsive: true,
      maintainAspectRatio: false,
      animation: {
        duration: 0,
      },
      interaction: {
        intersect: false,
        axis: 'x' as 'x',
        mode: 'index' as 'index',
      },
      scales: {
        x: {
          title: {
            display: true,
            align: 'center',
            text: 'Time',
            font: {
              size: 14,
            },
          },
          type: 'time' as 'time',
          time: {
            displayFormats: {
              millisecond: 'HH:mm:ss',
              minute: 'HH:mm:ss',
              hour: 'HH:mm',
              day: 'DD HH:mm',
            },
          },
          min: props.start.getTime(),
          max: props.end.getTime(),
          grid: {
            display: true,
            color: props.isDarkMode
              ? 'rgba(255, 255, 255, 0.3)'
              : 'rgba(0, 0, 0, 0.1)',
          },
        },
        y: {
          title: {
            display: true,
            align: 'center',
            text: `Data Usage (${UNITS[unitIndex]})`,
            font: {
              size: 14,
            },
          },
          min: 0,
          max: 1,
          display: true,
          grid: {
            display: true,
            color: props.isDarkMode
              ? 'rgba(255, 255, 255, 0.3)'
              : 'rgba(0, 0, 0, 0.1)',
          },
        },
      },
      plugins: {
        title: {
          display: true,
          text: 'Data Usage per Application every Interval',
          align: 'start' as 'start',
          font: { weight: 'bold', size: 16 },
          padding: {
            bottom: 10,
          },
        },
        legend: {
          display: true,
        },
        tooltip: { enabled: true },
        zoom: {
          pan: {
            enabled: true,
            mode: 'xy' as 'xy',
          },
          zoom: {
            wheel: { enabled: true },
            pinch: { enabled: true },
            mode: 'xy' as 'xy',
            speed: 2,
          },
          limits: {
            x: {
              min: props.start.getTime(),
              max: props.end.getTime(),
            },
            y: {
              min: 'original' as 'original' | any,
              max: 'original' as 'original' | any,
            },
          },
        },
      },
    };

    let canvas = document.getElementById(
      'bc-app-data-usage-per-interval'
    ) as HTMLCanvasElement;
    if (canvas && data.labels.length > 0 && barChart) {
      const xLimits = {
        min: data.labels[0].getTime() - 1000,
        max: data.labels[data.labels.length - 1].getTime() + 1000,
      };

      options.scales.x.min = xLimits.min;
      options.scales.x.max = xLimits.max;
      options.scales.y.max = yMax;
      options.plugins.zoom.limits.x = xLimits;
      options.plugins.zoom.limits.y.max = yMax;
      //@ts-ignore
      barChart.options = options;
      barChart.data = data;
      barChart.update();
      return;
    }

    const wrapper = document.getElementById(
      'wrapper-bc-app-data-usage-per-interval'
    );
    if (wrapper === null) return;
    wrapper.innerHTML = `<canvas id="bc-app-data-usage-per-interval" />`;

    canvas = document.getElementById(
      'bc-app-data-usage-per-interval'
    ) as HTMLCanvasElement;
    if (canvas === null) return;

    let ctx = canvas.getContext('2d');
    if (ctx === null) return;

    barChart = new Chart(ctx, {
      type: 'bar',
      data: data,
      //@ts-ignore
      options: options,
    });
  };

  return (
    <div className="block-chart">
      <div className="absolute top-2 right-4 text-left flex space-x-4 items-center">
        <Menu as="div" className="">
          <div>
            <Menu.Button className="dropdown-menu-button">
              {interval}
              <ChevronDownIcon
                className="-mr-1 ml-2 h-5 w-5"
                aria-hidden="true"
              />
            </Menu.Button>
          </div>

          <Transition as={Fragment}>
            <Menu.Items className="dropdown-menu-items">
              <div className="py-1">
                {INTERVALS.map((interval, i) => (
                  <Menu.Item key={interval}>
                    <a
                      className="dropdown-menu-item"
                      onClick={() => setInterval(INTERVALS[i])}
                    >
                      {interval}
                    </a>
                  </Menu.Item>
                ))}
              </div>
            </Menu.Items>
          </Transition>
        </Menu>

        <button
          className="button-unit"
          onClick={() => setUnitIndex((unitIndex + 1) % UNITS.length)}
        >
          {UNITS[unitIndex]}
        </button>
      </div>

      <div
        id="wrapper-bc-app-data-usage-per-interval"
        className="w-full h-[325px]"
        onDoubleClick={() => barChart.resetZoom()}
      ></div>
    </div>
  );
}

export default BarChartAppDataUsagePerInterval;
