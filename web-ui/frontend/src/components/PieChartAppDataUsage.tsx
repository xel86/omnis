import { useEffect, useState } from 'react';
import { AppSession } from '../interfaces';

import Chart from 'chart.js/auto';
import { ChartDataset, ChartOptions } from 'chart.js';

const UNITS = ['B', 'KB', 'MB', 'GB'];

interface PieChartAppDataUsageProps {
  appSessions: AppSession[];
  start: Date;
  end: Date;
  isDarkMode: boolean;
}

function convertToUnit(bytes: number, unitIndex: number): number {
  switch (unitIndex) {
    case 0:
      return bytes;
    case 1:
      return bytes / 1000;
    case 2:
      return bytes / 1000000;
    case 3:
      return bytes / 1000000000;
    default:
      return bytes;
  }
}

let pieChart: Chart;

function PieChartAppDataUsage(props: PieChartAppDataUsageProps) {
  const [data, setData] = useState({
    labels: [] as string[],
    datasets: [] as ChartDataset[],
  });
  const [unitIndex, setUnitIndex] = useState(2);

  useEffect(() => {
    const tmpData = {
      labels: [] as string[],
      datasets: [] as ChartDataset[],
    };

    const dataset = {
      label: 'Data Usage per Application',
      data: [] as number[],
      backgroundColor: [] as string[],
      hoverOffset: 4,
    };

    props.appSessions.forEach((as) => {
      tmpData.labels.push(as.application.name);
      dataset.backgroundColor?.push(as.application.colorHex);
      let sum = 0;
      as.sessions.forEach((s) => {
        sum += s.bytesRx + s.bytesTx;
      });
      dataset.data.push(convertToUnit(sum, unitIndex));
    });

    tmpData.datasets.push(dataset);
    setData(tmpData);
  }, [props.appSessions, props.isDarkMode, unitIndex]);

  useEffect(() => {
    buildChart();
  }, [data, unitIndex]);

  const buildChart = () => {
    const options: ChartOptions = {
      animation: {
        duration: 0,
      },
      plugins: {
        title: {
          display: true,
          text: 'Data Usage per Application',
          font: {
            weight: 'bold',
            size: 16,
          },
        },
      },
    };

    let canvas = document.getElementById(
      'pc-app-data-usage'
    ) as HTMLCanvasElement;
    if (canvas && data.labels.length > 0 && pieChart) {
      pieChart.data = data;
      pieChart.options = options;
      pieChart.update();
      return;
    }

    const wrapper = document.getElementById('wrapper-pc-app-data-usage');
    if (wrapper === null) return;
    wrapper.innerHTML = `<canvas id="pc-app-data-usage" />`;

    canvas = document.getElementById('pc-app-data-usage') as HTMLCanvasElement;
    if (canvas === null) return;

    let ctx = canvas.getContext('2d');
    if (ctx === null) return;

    pieChart = new Chart(ctx, {
      type: 'pie',
      data: data,
      options: options,
    });
  };

  return (
    <div className="block-chart">
      <button
        className="button-unit absolute top-4 right-4"
        onClick={() => setUnitIndex((unitIndex + 1) % UNITS.length)}
      >
        {UNITS[unitIndex]}
      </button>
      <div id="wrapper-pc-app-data-usage"></div>
    </div>
  );
}

export default PieChartAppDataUsage;
