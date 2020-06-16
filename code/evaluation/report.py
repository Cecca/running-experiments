import sqlite3
import matplotlib.pyplot as plt
import numpy as np

conn = sqlite3.connect('demo-db.sqlite')
c = conn.cursor()

datasets = [row[0] for row in c.execute('select distinct(dataset) from results;')]

for ds in datasets:
    print(ds)
    plt.figure()
    stmt = 'select hostname, algorithm, parameters, running_time_ns, recall from results where dataset=?'
    data = [row for row in c.execute(stmt, (ds,))]
    data.sort(key=lambda x: x[3])
    plt.barh(np.arange(len(data)), [x[3] / 1e9 for x in data])
    plt.yticks(np.arange(len(data)), [str(x[:3]) for x in data])
    plt.savefig('%s-running-time.png' % ds, bbox_inches='tight')

    plt.figure()
    data.sort(key=lambda x: x[4])
    plt.barh(np.arange(len(data)), [x[4] for x in data])
    plt.yticks(np.arange(len(data)), [str(x[:3]) for x in data])
    plt.savefig('%s-recall.png' % ds, bbox_inches='tight')
