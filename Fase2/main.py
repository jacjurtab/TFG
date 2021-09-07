import pandas as pd
import re
import json
import pymongo
from matplotlib import pyplot as plt
import deteccionAnomalias
from sklearn.metrics import confusion_matrix

# Leemos el fichero salida del disector de paquetes DNP3
df = pd.read_csv('extracciones_prueba.csv', sep=',')

# Leemos el fichero de marcas de tiempo de cada paquete DNP3
colum_time = pd.read_csv('Timestamp.csv')

# Añadimos una columna a nuestro fichero resultado del disector que es la marca de tiempo del paquete
df['Timestamp'] = colum_time
df['Timestamp'] = df['Timestamp'].multiply(1000)
df['Timestamp'] = pd.to_datetime(df['Timestamp'], unit='ms')

# Creamos otro dataframe que va a ser el fichero ya formateado
df_export = pd.DataFrame(columns=['NumeroPaquete','Timestamp','Origen','Destino','Longitud','CodigoFuncion','Objeto','Valor'])

# Iteramos por cada paquete (cada fila)
for row in df.iterrows():

    #Extraccion a variable de todas las columnas del fichero original
    numpqt = row[1]['NumeroPaquete']
    time = row[1]['Timestamp']
    origen = row[1]['Origen']
    destino = row[1]['Destino']
    len = row[1]['Longitud']
    fc = row[1]['CodigoFuncion']
    payload = row[1]['CampoVector']

    # Si el campo payload está vacío significa que es un paquete sin objetos
    if (payload == 'ND'):

        # Para evitar los valores NaN por defecto los campos vacíos se rellenan con la cadena ND
        # Añadimos al segundo dataframe creado (el fichero ya formateado) una fila idéntica a la que había en el primer dataframe
        df_export = df_export.append({'NumeroPaquete': numpqt, 'Timestamp': time, 'Origen': origen, 'Destino': destino, 'Longitud': len, 'CodigoFuncion': fc, 'Objeto': 'ND',
         'Valor': 'ND'}, ignore_index=True)

    # Si el paquete contiene objetos
    else:

        # Averiguamos el numero de objetos que contiene ese paquete
        numObjetos = row[1]['NumeroObjetos']

        # Averiguamos el numero de puntos que obtiene cada paquete
        # Esta sentencia devuelve un array con el valor del entero que queda encerrado entre la cadena 'numeroPuntos= ;'
        # de todas las veces (findall) que aparece en la variable payload
        num_puntos = re.findall('numeroPuntos=(\d?);', payload, re.DOTALL)

        # Separamos el campo payload en objeto, valor, valor, objeto ... aprovechando que usamos un delimitador diferente
        objetos = payload.split(';')

        # Si nos encontramos ante un paquete de solicitud no va a haber valores solo objetos
        if not num_puntos:
            j = 0
            for i in range(1, numObjetos + 1):
                df_export = df_export.append(
                    {'NumeroPaquete': numpqt, 'Timestamp': time, 'Origen': origen, 'Destino': destino, 'Longitud': len,
                     'CodigoFuncion': fc, 'Objeto': objetos[j],
                     'Valor': objetos[j+1]}, ignore_index=True)
                j = j + 2
        else:
            k = 0
            for i in range(1, numObjetos + 1):
                for j in range (1, int(num_puntos[i - 1]) + 1):
                    df_export = df_export.append(
                        {'NumeroPaquete': numpqt, 'Timestamp': time, 'Origen': origen, 'Destino': destino, 'Longitud': len,
                         'CodigoFuncion': fc, 'Objeto': objetos[k],
                         'Valor': objetos[k + j + 1]}, ignore_index=True)
                k = k + int(num_puntos[i-1]) + 2

df_export.to_csv('formattedfile.csv', index=False)


# A continuación insertar en la base de datos en una nueva coleccion el fichero formattedfile.csv
mng_client = pymongo.MongoClient('localhost', 27017)
mng_db = mng_client['dnp3']  # Replace mongo db name
collection_name = 'Datos'  # Replace mongo db collection name
db_cm = mng_db[collection_name]

data = pd.read_csv('formattedfile.csv')
data_json = json.loads(data.to_json(orient='records'))
db_cm.insert_many(data_json)



# Extraer datos
filter = {
    'Objeto': '32-Bit Analog Change Event w/o Time (Obj:32  Var:01)'
}
# sort=list({
#    'Timestamp': 1
# }.items())

# Realizamos la consulta a la base de datos
df_query = deteccionAnomalias.read_mongo('dnp3', 'Datos', filter, 'localhost', 27017)

# Deteccion de anomalias en un dato
# Ventana para el rolling
# ventana = 10

# Columna a analizar
columna = 'Valor'

# Convertimos a entero el valor de la columna valor
df_query['Valor'] = pd.to_numeric(df_query['Valor'])

# Porcentaje para detección
# k_deteccion = 1.05
###########################

for ventana in [3, 5, 8, 10]:
    for k_deteccion in [1.05, 1.10]:

        # calcular media móvil y su varianza
        df_query['mean'] = df_query[columna].rolling(ventana).mean()
        df_query['std'] = df_query[columna].rolling(ventana).std()

        # desplazar media y desvianza
        df_query['mean'] = df_query['mean'].shift(1)
        df_query['std'] = df_query['std'].shift(1)

        # Añadir columna anomaly
        df_query['anomaly'] = False

        # Añadir columna umbral
        df_query.loc[:, 'umbral'] = (df_query['mean'] + df_query['std']) * k_deteccion

        # Detección
        df_query.loc[df_query['Valor'] > (df_query['mean'] + df_query['std']) * k_deteccion, 'anomaly'] = True

        # Comprobamos si los resultados han sido falsos positivos, falsos negativos, verdaderos positivos o
        # verdaderos negativos
        try:
            tn, fp, fn, tp = confusion_matrix(df_query['AnomalyDB'], df_query['anomaly']).ravel()
        except:
            tn = confusion_matrix(df_query['AnomalyDB'], df_query['anomaly'])[0][0]
            fp = 0
            fn = 0
            tp = 0

        # Representacion de los resultados
        df_query['Valor'].plot()
        df_query['umbral'].plot()
        plt.title('Valor del punto y umbral permitido')
        plt.xlabel('Tiempo')
        plt.ylabel('Valor')
        plt.show()

        # Realizacion de matriz de confusion
        name = ['tn', 'fp', 'fn', 'tp']
        values = [tn, fp, fn, tp]
        plt.bar(name, values)
        plt.title('Matriz de confusion')
        plt.xlabel('Verdadero negativo/ Falso positivo/ Falso negativo/ Verdadero positivo')
        plt.ylabel('Total')
        plt.show()
