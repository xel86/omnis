import { useEffect, useState } from 'react';
import { AppSession, Session } from '../interfaces';

import Chart from 'chart.js/auto';
import { ChartDataset } from 'chart.js';
import zoomPlugin from 'chartjs-plugin-zoom';
import 'chartjs-adapter-moment';

interface LineChartAppUdp {
  appSessions: AppSession[];
  start: Date;
  end: Date;
  isDarkMode: boolean;
}

let lineChart: Chart;

function addAlpha(color: string, opacity: number): string {
  const _opacity = Math.round(Math.min(Math.max(opacity || 1, 0), 1) * 255);
  return color + _opacity.toString(16).toUpperCase();
}

function LineChartAppUdp(props: LineChartAppUdp) {
  const [data, setData] = useState({
    labels: [] as Date[],
    datasets: [] as ChartDataset[],
  });
  const [yLimits, setYLimits] = useState({ min: 0, max: 1 });

  useEffect(() => {
    // Populate all possible label values as Unix time for faster comparison
    const tmpLabels: number[] = [];
    props.appSessions.forEach((appSess) => {
      appSess.sessions.forEach((sess) => {
        if (tmpLabels.length === 0) tmpLabels.push(sess.start);
        if (!tmpLabels.includes(sess.start)) tmpLabels.push(sess.start);
      });
    });

    const tmpData = { labels: [] as Date[], datasets: [] as ChartDataset[] };
    tmpLabels.forEach((l) => tmpData.labels.push(new Date(l))); // Push labels into tmpData as Date type

    let max = 0;
    let min = 0;
    props.appSessions.forEach((appSess) => {
      const color: string = appSess.application.colorHex;
      const dataset = {
        label: appSess.application.name,
        data: [] as number[],
        borderColor: color,
        backgroundColor: addAlpha(color, 0.5),
      };

      const data: number[] = [];
      tmpLabels.forEach((label) => {
        const s: Session | undefined = appSess.sessions.find(
          (s) => s.start - label <= s.durationSec
        );

        if (s) {
          data.push(s.pktUdp);
          if (s.pktUdp > max) max = s.pktUdp;
          else if (s.pktUdp < min) min = s.pktUdp;
          else if (min === 0) min = s.pktUdp;
        } else data.push(0);
      });
      dataset.data = data;
      tmpData.datasets.push(dataset);
    });

    setYLimits({ min: Math.round(min * 0.8), max: Math.round(max * 1.2) });
    setData(tmpData);
  }, [props.appSessions, props.isDarkMode]);

  useEffect(() => {
    buildChart();
  }, [data]);

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
          type: 'time' as 'time',
          title: {
            display: true,
            align: 'center',
            text: 'Time',
            font: {
              size: 14,
            },
          },
          time: {
            displayFormats: {
              millisecond: 'ss.SSS',
              minute: 'mm:ss',
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
            text: `Udp Packets`,
            font: {
              size: 14,
            },
          },
          min: 0,
          max: 100,
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
          text: 'UDP Packets per Application',
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

    let canvas = document.getElementById('lc-app-udp') as HTMLCanvasElement;
    if (canvas && data.labels.length > 0 && lineChart) {
      const xLimits = {
        min: data.labels[0].getTime() - 1000,
        max: data.labels[data.labels.length - 1].getTime() + 1000,
      };

      options.scales.x.min = xLimits.min;
      options.scales.x.max = xLimits.max;
      options.scales.y.min = yLimits.min;
      options.scales.y.max = yLimits.max;
      options.plugins.zoom.limits = {
        x: xLimits,
        y: yLimits,
      };
      // @ts-ignore
      lineChart.options = options;
      lineChart.data = data;
      lineChart.update();
      return;
    }

    const wrapper = document.getElementById('wrapper-lc-app-udp');
    if (wrapper === null) return;
    wrapper.innerHTML = `<canvas id="lc-app-udp" />`;

    canvas = document.getElementById('lc-app-udp') as HTMLCanvasElement;
    if (canvas === null) return;

    let ctx = canvas.getContext('2d');
    if (ctx === null) return;

    lineChart = new Chart(ctx, {
      type: 'line',
      data: data,
      // @ts-ignore
      options: options,
    });
  };

  return (
    <div className="block-chart">
      <div
        id="wrapper-lc-app-udp"
        className="w-full h-[325px]"
        onDoubleClick={() => lineChart.resetZoom()}
      ></div>
    </div>
  );
}

export default LineChartAppUdp;
