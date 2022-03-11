"""Script listing items potentally needing a `price_postapoc` key

It stores candidates in `reprice` and
extracts item ids and names in `ids_and_names`.
"""

from json_tools.util import import_data

(data, errors) = import_data()
reprice = [item for item in data
           if 'price' in item and 'price_postapoc' not in item]
ids_and_names = [(r['id'], r['name'].get('str') or r['name'].get('str_sp'))
                 for r in reprice if 'abstract' not in r]
