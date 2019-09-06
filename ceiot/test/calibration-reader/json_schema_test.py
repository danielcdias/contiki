import json

from jsonschema import validate


def main():
    schema = {
        "type": "object",
        "patternProperties": {
            "^[s][1]?[0-9]$": {
                "type": "integer"
            }
        },
        "maxProperties": 10,
        "additionalProperties": False
    }
    test = json.loads(
        '{"s1": 1001, "s2": 1002, "s3": 1003, "s4": 1003, "s5": 1003'
        ', "s6": 1003, "s7": 1003, "s8": 1003, "s9": 1003, "s10": 1003}')
    validate(instance=test, schema=schema)
    print(test)


if __name__ == "__main__":
    main()
