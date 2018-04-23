var pr = function() {
    console.log.apply(console, arguments);
};


(function() {

    'use strict';
    var app = angular.module('app', []);


    app.controller('MainCtrl', function($scope, $http) {

        window.gscope = $scope;
        $scope.cell_view = [];
        $scope.units = [];
        $scope.cells = [];
        $scope.selected_cell = null;
        $scope.selected_unit = null;


        var init = function() {
            $http.get('/cell').then(function(res) {
                $scope.cells = res.data.objects;
                var view = $scope.cell_view;
                $scope.cells.forEach(function(x) {
                    var row = x.no / 8 | 0;
                    var col = x.no % 8;
                    if (view[row] == undefined) {
                        view[row] = [];
                    }
                    view[row][col] = x;
                });
                view.forEach(function(row) {
                    row.reverse();
                });
                window.x12 = view;
            });
            $http.get('/unit').then(function(res) {
                // pr(res.data);
                $scope.units = res.data.objects.sortBy('name');
            });
        };


        $scope.change_unit_state = function(unit) {
            unit.new_permission = (unit.new_permission == null ? !unit.permission : null);
            $scope.cells.forEach(function(x) {
                if (!x.unit || x.unit.id != unit.id) { return; }
                if (unit.new_permission == null) {
                    x.new_state = null;
                }
                else {
                    x.new_state = (unit.new_permission ? 'on' : 'off');
                }
                $http.patch('/cell/{0}'.format(x.id), {new_state: x.new_state}).then(function(res) {
                    pr('patch', res.data);
                });
            });
            $http.patch('/unit/{0}'.format(unit.id), {new_permission: unit.new_permission}).then(function(res) {
                pr('patch', res.data);
            });
        };


        $scope.on_unit_select = function(unit) {
            if (unit == $scope.selected_unit) {
                $scope.selected_unit = null;
            }
            else {
                $scope.selected_unit = unit;
            }
        };


        $scope.on_unit_mouseenter = function(unit) {
            return;
            $scope.cells.forEach(function(x) {
                if (!x.unit || x.unit.id != unit.id) { return; }
                x.highlight = true;
            });
        };


        $scope.on_unit_mouseleave = function(unit) {
            return;
            $scope.cells.forEach(function(x) {
                if (!x.unit || x.unit.id != unit.id) { return; }
                x.highlight = false;
            });
        };


        $scope.add_cell_to_unit = function(unit) {
            var cell = $scope.selected_cell;
            if (cell == null) { return; }
            pr(unit);
            pr(cell);
            $http.post('/assign-cell-to-unit/{0}/{1}'.format(unit.id, cell.id)).then(function(res) {
                pr(res.data);
            });
        };


        $scope.on_select_cell = function(cell) {
            if (cell.state != 'free') { return; }
            if (cell == $scope.selected_cell) {
                $scope.selected_cell = null;
                return;
            }
            $scope.selected_cell = cell;
        };


        init();

    });

})();
