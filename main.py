import pandas as pd
import json
from pymongo import MongoClient

df = pd.read_csv('extracciones_prueba.csv', error_bad_lines=False, sep=',')

# For de todas las filas donde el numero de objetos sea mayor que 1 hay que dividir el paquete en una fila
# para cada objeto (dejando igual) y anidar otro for para todas estas nuevas filas donde si cada objeto
# tiene mas de un punto (un valor) separar en una fila por cada objeto y valor de modo que el formato queda:
# NumeroPaquete Origen Destino Longitud CodigoFuncion Objeto Valor
# eliminamos (numero de objetos y numero de puntos)
# for index, row in df.iterrows():
#     print(row)

# Despues simplemente enviar el resultado a otro fichero csv
# df.to_csv('formattedfile.csv', index=False)

# A continuación insertar en la base de datos en una nueva coleccion el fichero formattedfile.csv
#     filepath = 'formattedfile.csv'
#     mng_client = pymongo.MongoClient('localhost', 27017)
#     mng_db = mng_client['dnp3']  # Replace mongo db name
#     collection_name = 'Datos'  # Replace mongo db collection name
#     db_cm = mng_db[collection_name]
#     cdir = os.path.dirname(__file__)
#     file_res = os.path.join(cdir, filepath)
#
#     data = pd.read_csv(file_res)
#     data_json = json.loads(data.to_json(orient='records'))
#     db_cm.remove()
#     db_cm.insert(data_json)


print(df.head())
