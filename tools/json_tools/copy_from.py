#!/usr/bin/env python3
'''
Fill in missing data from parents by `copy-from` field inheritance
'''

import sys

import inheritson

from util import import_data

LOADER_TYPE_FIELD = 'type'  # readability
INHERITANCE_FIELD = 'copy-from'

UNIQ_ID_FIELD = 'inheritson_id'
PARENT_UNIQ_ID_FIELD = 'inheritson_parent'


class DefaultLoader:
    '''
    '''
    id_field = 'id'
    assert_id = True

    def load(self, obj):
        '''
        '''
        obj = self.fix_id(obj)
        results = self.explode_id_lists(obj)
        for sub_obj in results:
            sub_obj = self.prepare(sub_obj)
        return results

    def fix_id(self, obj):
        '''
        '''
        return obj

    def explode_id_lists(self, obj):
        '''
        '''
        # TODO: fill looks_like with copy-from
        assert self.id_field in obj
        id_ = obj.get(self.id_field)
        if self.assert_id:
            assert id_

        if isinstance(id_, str):
            results = [obj]
        elif isinstance(id_, type(None)) and not self.assert_id:
            results = [obj]
        elif isinstance(id_, list):
            results = []
            for sub_id in id_:
                result = obj.copy()
                result[self.id_field] = sub_id
                results.append(result)
        else:
            raise ValueError(
                f'Unknown {self.id_field} data type: {id_data_type}')

        return results

    def prepare(self, obj):
        id_ = obj[self.id_field]
        type_ = obj[LOADER_TYPE_FIELD]
        obj[UNIQ_ID_FIELD] = (type_, id_)

        if INHERITANCE_FIELD in obj:
            obj[PARENT_UNIQ_ID_FIELD] = \
                (obj[LOADER_TYPE_FIELD], obj[INHERITANCE_FIELD])

        return obj

    def inherit(self, parent, child):
        '''
        TODO: expand, delete, looks_like
        '''
        assert parent, child
        result = parent.copy()
        result.update(child)
        return result


class AbstractLoader(DefaultLoader):
    '''
    '''
    abstract_field = 'abstract'

    def fix_id(self, obj):
        '''
        '''
        id_ = obj.get(self.id_field)
        abstract = obj.get(self.abstract_field)
        if self.assert_id:
            assert bool(id_) ^ bool(abstract)  # one and only one is filled
        obj[self.id_field] = id_ or abstract
        return obj


class EmptyIDLoader(AbstractLoader):
    '''
    '''
    assert_id = False


class VehiclePartLoader(AbstractLoader):
    pass


class RecipeLoader(AbstractLoader):
    '''
    '''
    id_field = 'result'  # TODO: compound results?


class UncraftLoader(RecipeLoader):
    pass


class OvermapTerrainLoader(EmptyIDLoader):
    pass


class MonsterLoader(AbstractLoader):
    pass


class MonstergroupLoader(AbstractLoader):
    id_field = 'name'


class ToolmodLoader(AbstractLoader):
    pass


class GenericLoader(AbstractLoader):
    pass


class BionicItemLoader(AbstractLoader):
    pass


class EngineLoader(AbstractLoader):
    pass


class ToolLoader(AbstractLoader):
    pass


class ComestibleLoader(AbstractLoader):
    pass


class MagazineLoader(AbstractLoader):
    pass


class GunLoader(AbstractLoader):
    pass


class BookLoader(AbstractLoader):
    pass


class PetArmorLoader(AbstractLoader):
    pass


class ArmorLoader(AbstractLoader):
    pass


class AmmoLoader(AbstractLoader):
    pass


class AsIsLoader:
    def load(self, obj):
        return obj

    def inherit(self, parent, child):
        assert None


class SpeechLoader(AsIsLoader):
    pass


class RotatableSymbolLoader(AsIsLoader):
    pass


class ProfessionItemSubstitutionsLoader(AsIsLoader):
    pass


class ObsoleteTerrainLoader(AsIsLoader):
    pass


class MonsterFactionLoader(AsIsLoader):
    pass


class HitRangeLoader(AsIsLoader):
    pass


class DreamLoader(AsIsLoader):
    pass


class MonsterBlacklistLoader(AsIsLoader):
    pass


class SnippetLoader(AsIsLoader):
    pass


class OvermapLandUseCodeLoader(AsIsLoader):
    pass


class ChargeRemovalBlacklistLoader(AsIsLoader):
    pass


class MapgenLoader(AsIsLoader):
    pass


class OverlayOrderLoader(AsIsLoader):
    pass


def get_loader_by_type(
        loader_type, default=DefaultLoader, raise_unknown=False):
    '''
    '''
    loader_type = loader_type.title().replace('_', '')
    try:
        return getattr(
            sys.modules[__name__],
            f'{loader_type}Loader')
    except AttributeError:
        if raise_unknown:
            raise ValueError(f'Unknown type {loader_type}')
        else:
            return default


def fill_data(data):
    '''
    '''
    to_fill = []
    skipped = []
    for obj in data:
        type_ = obj.get(LOADER_TYPE_FIELD)
        assert type_
        loader = get_loader_by_type(type_)()
        if isinstance(loader, AsIsLoader):
            skipped.append(obj)
        else:
            to_fill += loader.load(obj)

    filled = inheritson.fill(
        to_fill,
        id_field=UNIQ_ID_FIELD,
        parent_id_field=PARENT_UNIQ_ID_FIELD,
        raise_orphans=False,
        preserve_order=True,
        inheritance_function=loader.inherit)

    return filled + skipped


if __name__ == '__main__':
    print(fill_data(import_data()[0]))
