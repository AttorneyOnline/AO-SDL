import 'package:flutter_test/flutter_test.dart';

import 'package:ao_mobile/main.dart';

void main() {
  testWidgets('App widget builds', (WidgetTester tester) async {
    // The native bridge won't be available in unit tests, so we just
    // verify the widget tree can be instantiated.
    await tester.pumpWidget(const AoApp());
    expect(find.byType(AoApp), findsOneWidget);
  });
}
