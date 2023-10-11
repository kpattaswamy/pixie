import pymongo
import os

print("Starting MongoDB client")
#client = pymongo.MongoClient("mongodb://127.0.0.1:27017/")
client = pymongo.MongoClient("mongodb://localhost:27017/")

db = client["carDB"]
collection = db["car"]

# Insert document
car = { "name": "Honda" }
resp = collection.insert_one(car)
print(resp)

# Find document
resp = collection.find_one()
print(resp)

# Update document
newCar = { "$set": {"name": "Toyota"} }
resp = collection.update_one(car, newCar)
print(resp)

# Delete document
resp = collection.delete_one({"name": "Toyota"})
print(resp)