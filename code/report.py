import sqlite3
import matplotlib.pyplot as plt
import numpy as np

conn = sqlite3.connect('file:demo-db.sqlite')
c = conn.cursor()

datasets = [row[0] for row in c.execute('select distinct(dataset) from results;')]

for ds in datasets:
    print(ds)
    stmt = 'select algorithm, parameters, running_time_ns from results where dataset=?'
    running_times = [row for row in c.execute(stmt, (ds,))]
    plt.barh(np.arange(len(running_times)), [x[2] // 1000000000 for x in running_times])
    plt.yticks(np.arange(len(running_times)), [str(x[:2]) for x in running_times])
    plt.savefig('%s.png' % ds, bbox_inches='tight')
