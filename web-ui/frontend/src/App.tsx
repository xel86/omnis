import { Input } from 'reactstrap';
import { MdApps, MdDarkMode, MdLightMode } from 'react-icons/md';
import { useCallback, useEffect, useState } from 'react';
import { Chart } from 'chart.js';
import { AppSession, SessionTotal } from './interfaces';
import PaneRx from './components/PaneRx';
import PaneTx from './components/PaneTx';
import LineChartAppDataUsage from './components/LineChartAppDataUsage';
import BarChartAppDataUsagePerInterval from './components/BarChartAppDataUsageInterval';

const MS_MINUTE = 60000;

function randomHexColor(): string {
  return '#' + ((Math.random() * 0xffffff) << 0).toString(16).padStart(6, '0');
}

function getDateTimeString(date: Date): string {
  const offset = date.getTimezoneOffset();
  date = new Date(date.getTime() - offset * 60 * 1000);
  return date.toISOString().substring(0, 16);
}

async function getData(
  start: Date,
  end: Date
): Promise<AppSession[] | undefined> {
  const response: Response = await fetch(
    (process.env.REACT_APP_API_BASE_URL ?? 'http://localhost:29687') +
      '/data?' +
      new URLSearchParams({
        start: start.toISOString(),
        end: end.toISOString(),
      })
  ).catch((err: Error) => {
    throw err;
  });

  const body = await response.json();
  const appSessions: AppSession[] = body.data;
  if (response.ok) return appSessions;

  return Promise.resolve(undefined);
}

function App() {
  const [render, rerender] = useState(false);
  const [end, setEnd] = useState(new Date());
  const [start, setStart] = useState(new Date(end.getTime() - 5 * MS_MINUTE));
  const [isToPresent, setToPresent] = useState(false);
  const [appSessions, setAppSessions] = useState([] as AppSession[]);
  const [sessionTotal, setSessionTotal] = useState({
    bytesTx: 0,
    bytesRx: 0,
    pktTx: 0,
    pktRx: 0,
    pktTcp: 0,
    pktUdp: 0,
  } as SessionTotal);
  const [appColor, setAppColor] = useState(
    [] as { name: string; colorHex: string }[]
  );

  useEffect(() => {
    const getDataIntervalId = setInterval(async () => {
      if (isToPresent) {
        setEnd(new Date());
        updateData();
      }
    }, 5000);

    return () => clearInterval(getDataIntervalId);
  }, [start, end, isToPresent]);

  const updateData = useCallback(async () => {
    const appSessions = await getData(start, end);
    if (appSessions) {
      appSessions.forEach((as, idx) => {
        if (as.application.colorHex.length === 0) {
          let color = appColor.find(
            (ac) => ac.name === as.application.name
          )?.colorHex;

          if (!color) {
            color = randomHexColor();
            const arr = appColor;
            arr.push({ name: as.application.name, colorHex: color });
            setAppColor(arr);
          }

          appSessions[idx].application.colorHex = color;
        }
      });

      setAppSessions(appSessions);

      const sessionTotal: SessionTotal = {
        bytesTx: 0,
        bytesRx: 0,
        pktTx: 0,
        pktRx: 0,
        pktTcp: 0,
        pktUdp: 0,
      };

      appSessions.forEach((appSess) => {
        appSess.sessions.forEach((s) => {
          sessionTotal.bytesTx += s.bytesTx;
          sessionTotal.bytesRx += s.bytesRx;
          sessionTotal.pktTx += s.pktTx;
          sessionTotal.pktRx += s.pktRx;
          sessionTotal.pktTcp += s.pktTcp;
          sessionTotal.pktUdp += s.pktUdp;
        });
      });

      setSessionTotal(sessionTotal);
    }
  }, [start, end]);

  const isDarkMode =
    localStorage.getItem('theme') === 'dark' ||
    (!('theme' in localStorage) &&
      window.matchMedia('(prefers-color-scheme: dark)').matches);

  if (isDarkMode) {
    document.documentElement.classList.add('dark');
    Chart.defaults.color = '#fff';
  } else {
    document.documentElement.classList.remove('dark');
    Chart.defaults.color = '#666';
  }

  return (
    <div className="h-screen p-4 bg-gray-100 dark:bg-black">
      <div className="content-pane">
        <div className="w-full p-4 flex">
          <Input
            className="input-datetime"
            type="datetime-local"
            name="dateFrom"
            id="dateFrom"
            placeholder="datetime placeholder"
            value={getDateTimeString(start)}
            onChange={(e) => {
              setStart(new Date(e.target.value));
            }}
            max={getDateTimeString(end)}
          />
          <span className="flex items-center mx-4 text-xl font-semibold dark:text-white">
            -
          </span>
          <Input
            className="input-datetime"
            type="datetime-local"
            name="dateFrom"
            id="dateFrom"
            placeholder="datetime placeholder"
            value={getDateTimeString(end)}
            onChange={(e) => setEnd(new Date(e.target.value))}
            min={getDateTimeString(start)}
            disabled={isToPresent}
          />
          <button
            className={
              'text-sm min-w-max ml-2 px-4 rounded-lg font-semibold transition-all shadow-md ' +
              'dark:text-white ' +
              (isToPresent
                ? 'bg-blue-600 text-white dark:bg-gray-600 dark:text-white'
                : 'text-blue-600 hover:bg-gray-100 hover:text-blue-600 dark:hover:text-white dark:hover:bg-gray-600')
            }
            onClick={() => {
              setToPresent(!isToPresent);
              setEnd(new Date());
            }}
          >
            TO PRESENT
          </button>
          <button
            className="button-primary ml-4 font-semibold"
            onClick={updateData}
          >
            APPLY
          </button>

          <div className="w-full flex flex-row-reverse pl-4">
            <button
              className="ml-4 p-2"
              onClick={() => {
                isDarkMode
                  ? localStorage.setItem('theme', 'light')
                  : localStorage.setItem('theme', 'dark');
                rerender(!render);
              }}
            >
              {isDarkMode ? (
                <MdLightMode size={25} className="text-white" />
              ) : (
                <MdDarkMode size={25} />
              )}
            </button>

            <button className="button-primary font-bold">
              <MdApps className="mr-1" size={20} /> 20 APPS
            </button>
          </div>
        </div>

        <div className="content-grid">
          <div className="col-span-2">
            <PaneTx bytesTx={sessionTotal.bytesTx} pktTx={sessionTotal.pktTx} />
          </div>
          <div className="col-span-2">
            <PaneRx bytesRx={sessionTotal.bytesRx} pktRx={sessionTotal.pktRx} />
          </div>

          <div className="col-span-4">
            <LineChartAppDataUsage
              appSessions={appSessions}
              start={start}
              end={end}
              isDarkMode={isDarkMode}
            />
          </div>

          <div className="col-span-4">
            <BarChartAppDataUsagePerInterval
              appSessions={appSessions}
              start={start}
              end={end}
              isDarkMode={isDarkMode}
            />
          </div>

          <div className="col-span-2"></div>
          <div className="col-span-2"></div>

          <div className="col-span-4"></div>
        </div>
      </div>
    </div>
  );
}

export default App;
